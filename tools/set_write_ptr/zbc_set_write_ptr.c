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
#include <string.h>
#include <errno.h>

#include <libzbc/zbc.h>

#include <zbc_private.h>

/***** Main *****/

int main(int argc,
         char **argv)
{
    struct zbc_device_info info;
    struct zbc_device *dev;
    zbc_zone_t *zones = NULL;
    unsigned int nr_zones;
    long long lba;
    int i, z, ret = 1;
    char *path;

    /* Check command line */
    if ( argc < 4 ) {
usage:
        printf("Usage: %s [options] <dev> <zone number> <LBA (-1 for full)>\n"
               "Options:\n"
               "    -v   : Verbose mode\n",
               argv[0]);
        return( 1 );
    }

    /* Parse options */
    for(i = 1; i < (argc - 1); i++) {

        if ( strcmp(argv[i], "-v") == 0 ) {

            zbc_set_log_level("debug");

        } else if ( argv[i][0] == '-' ) {
            
            printf("Unknown option \"%s\"\n",
                   argv[i]);
            goto usage;

        } else {

            break;

        }

    }

    if ( i != (argc - 3) ) {
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
    ret = zbc_list_zones(dev, 0, ZBC_RO_ALL, &zones, &nr_zones);
    if ( ret != 0 ) {
        fprintf(stderr, "zbc_list_zones failed\n");
        ret = 1;
        goto out;
    }

    /* Check target zone */
    z = atoi(argv[i + 1]);
    if ( (z < 0) || (z > (int)nr_zones) ) {
        fprintf(stderr, "Invalid target zone number\n");
        ret = 1;
        goto out;
    }

    /* Get write pointer LBA */
    lba = strtoll(argv[i + 2], NULL, 10);
    if ( lba == -1 ) {
        lba = zones[z].zbz_start + zones[z].zbz_length;
    }

    printf("Setting zone %d/%d write pointer LBA to %llu...\n",
           z,
           nr_zones,
           (unsigned long long) lba);

    /* Set WP */
    ret = zbc_set_write_pointer(dev, zones[z].zbz_start, lba);
    if ( ret != 0 ) {
        fprintf(stderr,
                "zbc_set_write_pointer failed\n");
        ret = 1;
    }

out:

    if ( zones ) {
        free(zones);
    }

    /* Close device file */
    zbc_close(dev);

    return( ret );

}

