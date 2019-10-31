FROM 552819999234.dkr.ecr.us-east-1.amazonaws.com/hls-base:latest

COPY lasrc_sentinel_granule.sh ./usr/local/lasrc_sentinel_granule.sh

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["/usr/local/lasrc_sentinel_granule.sh"]

