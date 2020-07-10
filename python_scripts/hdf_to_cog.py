#! /usr/bin/env python
"""
Translate Sentinel-2 HLS HDF files to Cloud Optimized GeoTIFF aka COG.

Usage: hdf_to_cog.py [OPTIONS] INPUT

  Translate a file to a COG.

Options:
  --output-dir DIRECTORY          [required]

  -p, --cog-profile [jpeg|webp|zstd|lzw|deflate|packbits|lzma|lerc|lerc_deflate|lerc_zstd|raw]
                                  CloudOptimized GeoTIFF profile (default: deflate).

  --blocksize INTEGER             Overwrite internal tile size (default is set to 256).

  --co, --profile NAME=VALUE      Driver specific creation options. See the
                                  documentation for the selected output driver for more information.

  --help                          Show this message and exit.

Example:
$ python -m hdf_to_cog HLS.S30.T52SCG.2019253.v1.5.hdf --output-dir my-dir
or
$ python hdf_to_cog.py HLS.S30.T52SCG.2019253.v1.5.hdf --output-dir my-dir

"""

import os

import click

import rasterio
from rasterio.rio import options
from rio_cogeo.cogeo import cog_translate, cog_validate
from rio_cogeo.profiles import cog_profiles


S2_BAND_NAMES = (
    "B01",
    "B02",
    "B03",
    "B04",
    "B05",
    "B06",
    "B07",
    "B08",
    "B09",
    "B10",
    "B11",
    "B12",
    "B8A",
    "ACmask",
    "Fmask",
)


@click.command()
@options.file_in_arg
@click.option(
    "--output-dir",
    type=click.Path(dir_okay=True, file_okay=False, writable=True),
    required=True,
)
@click.option(
    "--cog-profile",
    "-p",
    "cogeo_profile",
    type=click.Choice(list(cog_profiles.keys())),
    default="deflate",
    help="CloudOptimized GeoTIFF profile (default: deflate).",
)
@click.option(
    "--blocksize",
    type=int,
    default=256,
    help="Overwrite internal tile size (default is set to 256).",
)
@options.creation_options
def main(input, output_dir, cogeo_profile, blocksize, creation_options):
    """Translate a file to a COG."""
    assert input.endswith(".hdf")

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    output_profile = cog_profiles.get(cogeo_profile)
    output_profile.update(
        dict(blockxsize=str(blocksize), blockysize=str(blocksize), predictor="2")
    )
    if creation_options:
        output_profile.update(creation_options)

    config = dict(GDAL_NUM_THREADS="ALL_CPUS", GDAL_TIFF_OVR_BLOCKSIZE="128")

    bname = os.path.splitext(os.path.basename(input))[0]
    with rasterio.open(input) as src_dst:

        assert len(src_dst.subdatasets) == len(S2_BAND_NAMES)

        for sds in src_dst.subdatasets:
            band = sds.split(":")[-1]
            assert band in S2_BAND_NAMES

            fname = "{}.{}.tif".format(bname, band)
            output_name = os.path.join(output_dir, fname)

            with rasterio.open(sds) as sub_dst:
                cog_translate(
                    sub_dst,
                    output_name,
                    output_profile,
                    config=config,
                    forward_band_tags=True,
                    overview_resampling="nearest",
                    quiet=True,
                )

            assert cog_validate(output_name)


if __name__ == "__main__":
    main()
