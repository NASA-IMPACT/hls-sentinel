## hls-sentinel
This repository contains the Dockerfiles for running the HLS sentinel granule code on ECS.

The `hls-sentinel` image uses [hls-base](https://github.com/NASA-IMPACT/hls-base/) as base image.

After building your base dependencies image and pushing it to ECR you can build the `hls-sentinel` processing image with:

```shell
$ docker build --build-arg AWS_ACCOUNT_ID=$AWS_ACCOUNT_ID -t hls-sentinel .
```

Note: The command above assumes you have exported an environment variable `AWS_ACCOUNT_ID` which references the AWS account where the hls-base reference image is stored.

You can then tag this `hls-sentinel` image as `<AWS_ACCOUNT_ID>.dkr.ecr.us-west-2.amazonaws.com/hls-sentinel` and push it to ECR.
