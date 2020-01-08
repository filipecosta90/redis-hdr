#!/bin/bash
exec lcov \
    --directory /Users/filipeoliveria/redislabs/redis_hdr/build \
    --base-directory /Users/filipeoliveria/redislabs/redis_hdr/src \
    -z
