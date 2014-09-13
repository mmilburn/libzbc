/*
 * This file is part of libzbc.
 * 
 * Copyright (C) 2009-2014, HGST, Inc.  This software is distributed
 * under the terms of the GNU Lesser General Public License version 3,
 * or any later version, "as is," without technical support, and WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  You should have received a copy
 * of the GNU Lesser General Public License along with libzbc.  If not,
 * see <http://www.gnu.org/licenses/>.
 * 
 * Authors: Damien Le Moal (damien.lemoal@hgst.com)
 *          Christophe Louargant (christophe.louargant@hgst.com)
 */

/***** Including files *****/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libzbc/zbc.h>

/***** Main *****/

int main(int argc,
         char **argv)
{
    struct zbc_device_info info;
    unsigned long long lba = 0;
    struct zbc_device *dev;
    enum zbc_reporting_options ro = ZBC_RO_ALL;
    int i, ret = 1;
    int num = 0;
    zbc_zone_t *zones = NULL;
    unsigned int nr_zones;
    char *path;

    /* Check command line */
    if ( argc < 2 ) {
usage:
        printf("Usage: %s [options] <dev>\n"
               "Options:\n"
               "    -v         : Verbose mode\n"
               "    -n         : Get only the number of zones\n"
               "    -lba <lba> : Specify zone start LBA (default is 0)\n"
               "    -ro <opt>  : Specify reporting option: \"all\", \"empty\",\n"
               "                 \"open\", \"rdonly\", \"full\", \"offline\"\n"
               "                 or \"reset\". Default is \"all\"\n",
               argv[0]);
        return( 1 );
    }

    /* Parse options */
    for(i = 1; i < (argc - 1); i++) {

        if ( strcmp(argv[i], "-v") == 0 ) {

            zbc_set_log_level("debug");

        } else if ( strcmp(argv[i], "-n") == 0 ) {

            num = 1;

        } else if ( strcmp(argv[i], "-lba") == 0 ) {

            if ( i >= (argc - 1) ) {
                goto usage;
            }
            i++;

            lba = strtoll(argv[i], NULL, 10);

        } else if ( strcmp(argv[i], "-ro") == 0 ) {

            if ( i >= (argc - 1) ) {
                goto usage;
            }
            i++;

            if ( strcmp(argv[i], "all") == 0 ) {
                ro = ZBC_RO_ALL;
            } else if ( strcmp(argv[i], "full") == 0 ) {
                ro = ZBC_RO_FULL;
            } else if ( strcmp(argv[i], "open") == 0 ) {
                ro = ZBC_RO_OPEN;
            } else if ( strcmp(argv[i], "empty") == 0 ) {
                ro = ZBC_RO_EMPTY;
            } else if ( strcmp(argv[i], "rdonly") == 0 ) {
                ro = ZBC_RO_RDONLY;
            } else if ( strcmp(argv[i], "offline") == 0 ) {
                ro = ZBC_RO_OFFLINE;
            } else if ( strcmp(argv[i], "reset") == 0 ) {
                ro = ZBC_RO_RESET;
            } else {
                fprintf(stderr, "Unknown zone reporting option \"%s\"\n",
                        argv[i]);
                goto usage;
            }

        } else if ( argv[i][0] == '-' ) {

            printf("Unknown option \"%s\"\n",
                   argv[i]);
            goto usage;

        } else {

            break;

        }

    }

    if ( i != (argc - 1) ) {
        goto usage;
    }

    /* Open device */
    path = argv[i];
    ret = zbc_open(path, O_RDONLY, &dev);
    if ( ret != 0 ) {
        return( 1 );
    }

    ret = zbc_get_device_info(dev, &info);
    if ( ret < 0 ) {
        fprintf(stderr,
                "zbc_get_device_info failed\n");
        goto out;
    }

    printf("Device %s: %s interface, %s disk model\n",
           path,
           zbc_disk_type_str(info.zbd_type),
           zbc_disk_model_str(info.zbd_model));
    printf("    %llu logical blocks of %u B\n",
           (unsigned long long) info.zbd_logical_blocks,
           (unsigned int) info.zbd_logical_block_size);
    printf("    %llu physical blocks of %u B\n",
           (unsigned long long) info.zbd_physical_blocks,
           (unsigned int) info.zbd_physical_block_size);

    /* Get zone list */
    ret = zbc_list_zones(dev, lba, ro, &zones, &nr_zones);
    if ( ret != 0 ) {
        fprintf(stderr, "zbc_list_zones failed\n");
        ret = 1;
        goto out;
    }

    /* Print zone info */
    printf("    %u zones from LBA %llu, reporting option 0x%02x\n",
           nr_zones,
           lba,
           ro);

    if ( ! num ) {
        for(i = 0; i < nr_zones; i++) {
            printf("Zone %05d: type 0x%x, cond 0x%x, need reset 0x%x, LBA %11llu, %11llu sectors, wp %11llu\n",
                   i,
                   zones[i].zbz_type,
                   zones[i].zbz_condition,
                   zones[i].zbz_need_reset,
                   (unsigned long long) zones[i].zbz_start,
                   (unsigned long long) zones[i].zbz_length,
                   (unsigned long long) zones[i].zbz_write_pointer);
        }
    }

out:

    if ( zones ) {
        free(zones);
    }

    zbc_close(dev);

    return( ret );

}
