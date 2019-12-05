## hls-sentinel
This repository contains the Dockerfiles for running the HLS sentinel granule code on ECS.

The `hls-sentinel` image uses [hls-base](https://github.com/NASA-IMPACT/hls-base/) as base image.

After building your base dependencies image and pushing it to ECR you can build the `hls-sentinel` processing image with

```shell
$ docker build --no-cache --tag hls-sentinel .
```
You can then tag this `hls-sentinel` image as `350996086543.dkr.ecr.us-west-2.amazonaws.com/hls-sentinel` and push it to ECR.
