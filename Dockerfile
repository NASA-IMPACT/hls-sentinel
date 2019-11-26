ARG AWS_ACCOUNT_ID=000000000000
FROM ${AWS_ACCOUNT_ID}.dkr.ecr.us-west-2.amazonaws.com/hls-base:latest

COPY lasrc_sentinel_granule.sh ./usr/local/lasrc_sentinel_granule.sh

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["/usr/local/lasrc_sentinel_granule.sh"]

