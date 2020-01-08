ADD_LIBRARY(redis_hdr_obj OBJECT
        redis_hdr.c
        hdr_histogram.c
        hdr_histogram_log.c
        )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

