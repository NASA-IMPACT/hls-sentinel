#! /usr/bin/env python
import sys, getopt
import os
import shutil
import glob

def main(argv):
    inputs2dir = ''
    try:
        opts, args = getopt.getopt(argv,"i:h",["inputs2dir="])
    except getopt.GetoptError:
        print 'get_s2_granule_dir.py -i <inputs2dir>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'get_s2_granule_dir.py -i <inputs2dir>'
            sys.exit()
        elif opt in ("-i", "--inputs2dir"):
            inputs2dir = arg

    # determine the name of the {product_id} directory under GRANULE
    granule_dir = os.path.join(inputs2dir, 'GRANULE')

    # check if it is an old format SAFE directory
    mtd_xml = os.path.join(inputs2dir, 'MTD_MSIL1C.xml')
    old_s2_format = False
    if not os.path.isfile(mtd_xml):
        old_s2_format = True
    found = False
    gran_dirs = glob.glob('{}/*'.format(granule_dir))
    for tmpdir in gran_dirs:
        # only look at the directories
        if os.path.isdir(tmpdir):
            # old S2 - looking for directories with S2[A|B]_OPER_MSI_L1C_TL*
            if old_s2_format and (tmpdir.find('OPER_MSI_L1C_TL') != -1):
                # found desired directory
                prodid_dir = tmpdir
                found = True
                break

            # new S2 - looking for directories with L1C_*
            elif not old_s2_format and (tmpdir.find('L1C_') != -1):
            # found desired directory
                prodid_dir = tmpdir
                found = True
                break

    # make sure the product_id directory was found
    if found:
        grandir_id = os.path.basename(os.path.normpath(prodid_dir))
        sys.stdout.write(grandir_id)
    else:
        sys.stdout.write("")

if __name__ == "__main__":
    main(sys.argv[1:])
