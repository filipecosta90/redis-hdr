#!/bin/sh

INFO_FILE=$1
if [ -z "${INFO_FILE}" ]; then
    INFO_FILE='coverage.info'
fi

LCOV=lcov
${LCOV} -c -d /Users/filipeoliveria/redislabs/redis_hdr -o ${INFO_FILE}
${LCOV} -r ${INFO_FILE} -o ${INFO_FILE} \
    '/Applications/*' \
    '/usr/*' \
    '/Users/filipeoliveria/redislabs/redis_hdr/deps/*' 

echo "Wrote coverage report to ${INFO_FILE}"
