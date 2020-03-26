#! /usr/bin/env python
import sys, getopt
import os
import shutil
import uuid
import json
import hashlib
from urlparse import urlparse

def main(argv):
    inputdir = ''
    outputfile = ''
    bucket = ''
    collection = ''
    product = ''
    manifest = {}
    try:
        opts, args = getopt.getopt(argv,"i:o:b:c:p:h",["inputdir=","outputfile=","bucket=","collection=","product",])
    except getopt.GetoptError:
        print 'create_manifest.py -i <inputdir> -o <outputfile> -b <bucket> -c <collection> -p <product>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'create_manifest.py -i <inputdir> -o <outputfile> -b <bucket> -c <collection> -p <product>'
            sys.exit()
        elif opt in ("-i", "--inputdir"):
            inputdir = arg
        elif opt in ("-o", "--outputfile"):
            outputfile = arg
        elif opt in ("-b", "--bucket"):
            bucket = arg
        elif opt in ("-c", "--collection"):
            collection = arg
        elif opt in ("-p", "--product"):
            product = arg

    manifest["collection"] = collection
    manifest["id"] = str(uuid.uuid4())
    manifest["version"] = "1.5"
    manifest["product"] = {"name": product}
    manifest["files"] = []
    for filename in os.listdir(inputdir):
        if filename.endswith(".tif") or filename.endswith(".jpg") or filename.endswith(".xml"):
            file_item = {}
            file_item["name"] = filename
            size = os.path.getsize(os.path.join(inputdir, filename))
            file_item["size"] = size
            with open(os.path.join(inputdir, filename), "rb") as f:
                file_hash = hashlib.md5()
                while True:
                    chunk = f.read(8192)
                    if not chunk:
                        break
                    file_hash.update(chunk)
            file_item["checksum"] = file_hash.hexdigest()
            file_item["checksum-type"] = "MD5"

            normal_bucket = urlparse(bucket).geturl()
            file_item["uri"] = "%s/%s" % (normal_bucket, filename)

            if filename.endswith(".tif"):
                file_item["type"] = "data"
            if filename.endswith(".xml"):
                file_item["type"] = "metadata"
                file_item["uri"] = "%sdata/%s" % (normal_bucket, filename)
            if filename.endswith(".jpg"):
                file_item["type"] = "browse"

            manifest["files"].append(file_item)
            continue
        else:
            continue
    with open(outputfile, 'w') as out:
        json.dump(manifest, out)

if __name__ == "__main__":
    main(sys.argv[1:])
