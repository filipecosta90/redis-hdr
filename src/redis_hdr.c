/*
 * Copyright (c) 2019 RedisLabs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <sys/time.h>
#include "redismodule.h"
#include "hdr_histogram.h"
#include "hdr_histogram_log.h"

static RedisModuleType *HDR_Data_Type;

#define HDR_ENCODING_VERSION 0
#define UNUSED(V) ((void)V)

int HDR_Init_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc)
{
  /* HDR.INIT <key> <lowest_trackable_value> <highest_trackable_value> <significant_figures> */
  if (argc != 5)
    return RedisModule_WrongArity(ctx);

  /* Parse and validate the arguments, in their order. */
  int64_t lowest_trackable_value = 0;
  int64_t highest_trackable_value = 0;
  int significant_figures = 0;

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  const int type = RedisModule_KeyType(key);

  if (!(type == REDISMODULE_KEYTYPE_MODULE &&
        RedisModule_ModuleTypeGetType(key) == HDR_Data_Type) &&
      type != REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (RedisModule_StringToLongLong(argv[2], &lowest_trackable_value) != REDISMODULE_OK)
  {
    return RedisModule_ReplyWithError(ctx, "ERR invalid lowest_trackable_value");
  }

  if (RedisModule_StringToLongLong(argv[3], &highest_trackable_value) != REDISMODULE_OK)
  {
    return RedisModule_ReplyWithError(ctx, "ERR invalid highest_trackable_value");
  }

  if (RedisModule_StringToLongLong(argv[4], (long long int *)&significant_figures) != REDISMODULE_OK && (significant_figures < 1 || significant_figures > 4))
  {
    return RedisModule_ReplyWithError(ctx, "ERR invalid significant_figures");
  }
  struct hdr_histogram *hist;
  RedisModule_Log(ctx, "notice", "loaded default compaction policy: %lld %lld %d\n\r", lowest_trackable_value, highest_trackable_value, (int)significant_figures);

  const int hdr_result = hdr_init(
      lowest_trackable_value,   // Minimum value
      highest_trackable_value,  // Maximum value
      (int)significant_figures, // Number of significant figures
      &hist);                   // Pointer to initialise

  if (hdr_result == 0)
  {
    const int set_reply = RedisModule_ModuleTypeSetValue(key, HDR_Data_Type, hist);
    RedisModule_CloseKey(key);
    if (set_reply != REDISMODULE_OK)
    {
      return RedisModule_ReplyWithError(ctx, "ERR error while saving HDR");
    }
  }
  else
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "ERR error while initing the HDR");
  }

  RedisModule_ReplyWithSimpleString(ctx, "OK");
  RedisModule_ReplicateVerbatim(ctx);
  return REDISMODULE_OK;
}

int HDR_RecordValue_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                 int argc)
{
  /* HDR.RECORDVALUE <key> <value1> [<value2>]... */
  if (argc < 3)
    return RedisModule_WrongArity(ctx);

  struct hdr_histogram *hist;

  /* Parse and validate the arguments, in their order. */
  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  const int type = RedisModule_KeyType(key);

  if (type == REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
  }
  else if (!(type == REDISMODULE_KEYTYPE_MODULE &&
             RedisModule_ModuleTypeGetType(key) == HDR_Data_Type))
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  else
  {
    hist = RedisModule_ModuleTypeGetValue(key);
    for (int i = 2; i < argc; i++)
    {
      long long value;
      if (RedisModule_StringToLongLong(argv[i], &value) != REDISMODULE_OK)
      {
        RedisModule_CloseKey(key);
        return RedisModule_ReplyWithError(ctx, "ERR invalid argument");
      }
      if (hdr_record_value(hist, value) == 0)
      {
        RedisModule_CloseKey(key);
        return RedisModule_ReplyWithError(ctx, "ERR error recording value");
      }
      RedisModule_Log(ctx, "notice", "added value %lld", value);
    }
  }
  RedisModule_ReplyWithSimpleString(ctx, "OK");
  RedisModule_CloseKey(key);
  RedisModule_ReplicateVerbatim(ctx);
  return REDISMODULE_OK;
}

int HDR_RecordValues_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                  int argc)
{
  /* HDR.RECORDVALUES <key> <value1> <count1> [<value2> <count2>]... */
  if (argc < 4)
    return RedisModule_WrongArity(ctx);

  struct hdr_histogram *hist;

  /* Parse and validate the arguments, in their order. */
  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  const int type = RedisModule_KeyType(key);

  if (type == REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
  }
  else if (!(type == REDISMODULE_KEYTYPE_MODULE &&
             RedisModule_ModuleTypeGetType(key) == HDR_Data_Type))
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  else
  {
    hist = RedisModule_ModuleTypeGetValue(key);
    for (size_t i = 2; i < argc; i++)
    {
      long long value;
      if (RedisModule_StringToLongLong(argv[i], &value) != REDISMODULE_OK)
      {
        RedisModule_CloseKey(key);
        return RedisModule_ReplyWithError(ctx, "ERR invalid argument");
      }
      if (hdr_record_value(hist, value) == 0)
      {
        RedisModule_CloseKey(key);
        return RedisModule_ReplyWithError(ctx, "ERR error recording value");
      }
    }
  }
  RedisModule_ReplyWithSimpleString(ctx, "OK");
  RedisModule_CloseKey(key);
  RedisModule_ReplicateVerbatim(ctx);
  return REDISMODULE_OK;
}

int HDR_Reset_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc)
{
  /* HDR.RESET <key> */
  if (argc != 2)
    return RedisModule_WrongArity(ctx);

  struct hdr_histogram *hist;

  /* Parse and validate the arguments, in their order. */
  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  const int type = RedisModule_KeyType(key);

  if (type == REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
  }
  else if (!(type == REDISMODULE_KEYTYPE_MODULE &&
             RedisModule_ModuleTypeGetType(key) == HDR_Data_Type))
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  else
  {
    hist = RedisModule_ModuleTypeGetValue(key);
    hdr_reset(hist);
  }
  RedisModule_CloseKey(key);
  RedisModule_ReplyWithSimpleString(ctx, "OK");
  RedisModule_ReplicateVerbatim(ctx);
  return REDISMODULE_OK;
}

int HDR_Add_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc)
{
  /* HDR.ADD <to> <from1> [<from2>]... */
  if (argc < 3)
    return RedisModule_WrongArity(ctx);

  struct hdr_histogram *to_hist;

  /* Parse and validate the arguments, in their order. */
  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  const int type = RedisModule_KeyType(key);

  if (type == REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
  }
  else if (!(type == REDISMODULE_KEYTYPE_MODULE &&
             RedisModule_ModuleTypeGetType(key) == HDR_Data_Type))
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  else
  {
    to_hist = RedisModule_ModuleTypeGetValue(key);
  }

  for (size_t i = 2; i < argc; i++)
  {
    struct hdr_histogram *from_hist;

    /* Parse and validate the arguments, in their order. */
    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY)
    {
      RedisModule_CloseKey(key);
      return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
    }
    else if (!(type == REDISMODULE_KEYTYPE_MODULE &&
               RedisModule_ModuleTypeGetType(key) == HDR_Data_Type))
    {
      RedisModule_CloseKey(key);
      return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    }
    else
    {
      to_hist = RedisModule_ModuleTypeGetValue(key);
    }
  }

  RedisModule_ReplyWithSimpleString(ctx, "OK");
  RedisModule_ReplicateVerbatim(ctx);
  RedisModule_CloseKey(key);
  return REDISMODULE_OK;
}

int HDR_ValueAtPercentile_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                       int argc)
{
  /* HDR.VALUEATP <key> <percentile1> [<percentile2>]... */
  if (argc < 3)
    return RedisModule_WrongArity(ctx);

  struct hdr_histogram *hist;

  /* Parse and validate the arguments, in their order. */
  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  const int type = RedisModule_KeyType(key);

  if (type == REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
  }
  else if (!(type == REDISMODULE_KEYTYPE_MODULE &&
             RedisModule_ModuleTypeGetType(key) == HDR_Data_Type))
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  hist = RedisModule_ModuleTypeGetValue(key);
  const long total_results = argc - 2;
  double percentile;
  long long value;
  RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
  long len = 0;
  for (int i = 2; i < argc; i++)
  {
    if (RedisModule_StringToDouble(argv[i], &percentile) != REDISMODULE_OK)
    {
      RedisModule_CloseKey(key);
      return RedisModule_ReplyWithError(ctx, "ERR invalid percentile");
    }
    value = hdr_value_at_percentile(hist, percentile);
    RedisModule_ReplyWithDouble(ctx, percentile);
    RedisModule_ReplyWithLongLong(ctx, value);
    len = len + 2;
  }
  RedisModule_ReplySetArrayLength(ctx, len);

  RedisModule_CloseKey(key);
  return REDISMODULE_OK;
}

int HDR_EncodeCompressed_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                      int argc)
{
  /* HDR.ENCODECOMP <key>*/
  if (argc != 2)
    return RedisModule_WrongArity(ctx);

  struct hdr_histogram *hist;

  /* Parse and validate the arguments, in their order. */
  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  const int type = RedisModule_KeyType(key);

  if (type == REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
  }
  else if (!(type == REDISMODULE_KEYTYPE_MODULE &&
             RedisModule_ModuleTypeGetType(key) == HDR_Data_Type))
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  hist = RedisModule_ModuleTypeGetValue(key);
  char *encoded_histogram = NULL;
  size_t encoded_len = 0;
  const int result = hdr_encoded_histogram_format(hist, &encoded_histogram,
                                                  &encoded_len);
  if (result != 0)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "ERR error encoding histogram");
  }
  RedisModule_CloseKey(key);

  RedisModule_ReplyWithSimpleString(ctx, "OK");
  return REDISMODULE_OK;
}

int HDR_DecodeCompressed_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                      int argc)
{
  /* HDR.DECODECOMP <key> <compressed_histogram> */
  if (argc != 3)
    return RedisModule_WrongArity(ctx);

  struct hdr_histogram *hist;

  /* Parse and validate the arguments, in their order. */
  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, "HDR: key does not exist");
  }
  else if (RedisModule_ModuleTypeGetType(key) != HDR_Data_Type)
  {
    RedisModule_CloseKey(key);
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  else
  {
    hist = RedisModule_ModuleTypeGetValue(key);
  }

  RedisModule_CloseKey(key);

  RedisModule_ReplyWithSimpleString(ctx, "OK");
  return REDISMODULE_OK;
}

// Releases HDR and all its compaction rules
void HDR_free(void *value)
{
  struct hdr_histogram *hist = (struct hdr_histogram *)value;
  hdr_close(hist);
}

size_t HDR_mem_usage(const void *value)
{
  struct hdr_histogram *hist = (struct hdr_histogram *)value;
  return hdr_get_memory_size(hist);
}

void HDR_RdbSave(RedisModuleIO *rdb, void *value)
{
  struct hdr_histogram *hist = (struct hdr_histogram *)value;
  if (hist != NULL)
  {
    uint8_t *compressed_histogram = NULL;
    int compressed_len = 0;
    int rc = hdr_encode_compressed(hist, &compressed_histogram, &compressed_len);
    if (rc != 0)
    {
      RedisModule_LogIOError(rdb, "warning", "While saving the HDR hdr_encode_compressed returned error.");
    }
    /* Save the hdr compressed length. */
    RedisModule_SaveSigned(rdb, compressed_len);
    /* Save the hdr compressed bytes. */
    for (int j = 0; j < compressed_len; j++)
    {
      RedisModule_SaveUnsigned(rdb, compressed_histogram[j]);
    }
  }
}

void *HDR_RdbLoad(RedisModuleIO *rdb, int encver)
{
  /* As long as the module is not stable, we don't care about
     * loading old versions of the encoding. */
  if (encver != HDR_ENCODING_VERSION)
  {
    RedisModule_LogIOError(rdb, "warning", "Sorry the HDR Redis module only supports RDB files written with the encoding version %d. This file has encoding version %d, and was likely written by a previous version of this module that is now deprecated. ", HDR_ENCODING_VERSION, encver);
    return NULL;
  }

  return NULL;
}

void HDR_AofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value)
{
  UNUSED(aof);
  UNUSED(key);
  UNUSED(value);
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv,
                       int argc)
{
  REDISMODULE_NOT_USED(argv);
  REDISMODULE_NOT_USED(argc);

  if (RedisModule_Init(ctx, "hdr", 1, REDISMODULE_APIVER_1) ==
      REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  RedisModuleTypeMethods tm = {
      .version = REDISMODULE_TYPE_METHOD_VERSION,
      .rdb_load = HDR_RdbLoad,
      .rdb_save = HDR_RdbSave,
      .aof_rewrite = HDR_AofRewrite,
      .mem_usage = HDR_mem_usage,
      .free = HDR_free};

  HDR_Data_Type = RedisModule_CreateDataType(ctx, "hellotype", HDR_ENCODING_VERSION, &tm);
  if (HDR_Data_Type == NULL)
  {
    RedisModule_Log(ctx, "warning", "Can not initialize HDR DataType\r\n");
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.init", HDR_Init_RedisCommand,
                                "write deny-oom fast", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.recordvalue", HDR_RecordValue_RedisCommand,
                                "write deny-oom fast", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.recordvalues", HDR_RecordValues_RedisCommand,
                                "write deny-oom fast", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.reset", HDR_Reset_RedisCommand,
                                "write deny-oom fast", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.valueatp", HDR_ValueAtPercentile_RedisCommand,
                                "readonly fast", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.add", HDR_Add_RedisCommand,
                                "write deny-oom", 0, 0,
                                0) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.encodecomp", HDR_Add_RedisCommand,
                                "write deny-oom random", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "hdr.decodecomp", HDR_Add_RedisCommand,
                                "write deny-oom random", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  return REDISMODULE_OK;
}
