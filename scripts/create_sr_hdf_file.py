#! /usr/bin/env python
import sys, getopt
import urllib2
from espa import XMLError, XMLInterface
from espa import MetadataError, Metadata

def main(argv):
    inputxmlfile = ''
    outputxmlfile = ''
    hdffile = ''
    try:
        opts, args = getopt.getopt(argv,"i:o:f:h",["inputxmlfile=","outputxmlfile=","hdffile="])
    except getopt.GetoptError:
        print 'create_sr_hdf_file.py -i <inputxmlfile> -o <outputxmlfile> -f <hdffile>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'test.py -i <inputxmlfile> -o <outputxmlfile> -f <hdffile>'
            sys.exit()
        elif opt in ("-i", "--inputxmlfile"):
            inputxmlfile = arg
        elif opt in ("-o", "--outputxmlfile"):
            outputxmlfile = arg
        elif opt in ("-f", "--hdffile"):
            hdffile = arg
    mm = Metadata(xml_filename=inputxmlfile)
    mm.parse()
    hls_product = 'hls'
    if hdffile == 'one':
        for band in mm.xml_object.bands.iterchildren():
            if band.get('name') == 'sr_band1':
                band.set('name', 'band01')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band2':
                band.set('name', 'blue')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band3':
                band.set('name', 'green')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band4':
                band.set('name', 'red')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band5':
                band.set('name', 'band05')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band6':
                band.set('name', 'band06')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band7':
                band.set('name', 'band07')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band8':
                band.set('name', 'band08')
                band.set('product', hls_product)
    elif hdffile == 'two':
        for band in mm.xml_object.bands.iterchildren():
            if band.get('name') == 'sr_band8a':
                band.set('name', 'band8a')
                band.set('product', hls_product)
            elif band.get('name') == 'B09':
                band.set('name', 'band09')
                band.set('product', hls_product)
            elif band.get('name') == 'B10':
                band.set('name', 'band10')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band11':
                band.set('name', 'band11')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_band12':
                band.set('name', 'band12')
                band.set('product', hls_product)
            elif band.get('name') == 'sr_aerosol':
                band.set('name', 'CLOUD')
                band.set('product', hls_product)

    for band in mm.xml_object.bands.iterchildren():
        if band.get('product') != hls_product:
            print(band.get('name'))
            mm.xml_object.bands.remove(band)

    mm.write(xml_filename=outputxmlfile)

if __name__ == "__main__":
    main(sys.argv[1:])
