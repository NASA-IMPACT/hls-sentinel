#! /usr/bin/env python
import sys, getopt
import libxml2


def main(argv):
    input_file = ""
    valid = "valid" 
    try:
        opts, args = getopt.getopt(argv,"i:h",["input_file="])
    except getopt.GetoptError:
        print ("check_solar_zenith.py -i MTD_TL.xml")
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-h":
            print ('check_solar_zenith.py -i MTD_TL.xml')
            sys.exit()
        elif opt in ("-i", "--input_file"):
            input_file = arg
    doc = libxml2.parseFile(input_file)
    context = doc.xpathNewContext()
    element = context.xpathEval("//Mean_Sun_Angle/ZENITH_ANGLE")
    solar_zenith = float(element[0].content)
    if (solar_zenith > 76):
        valid = "invalid"
    sys.stdout.write(valid)

if __name__ == "__main__":
    main(sys.argv[1:])
