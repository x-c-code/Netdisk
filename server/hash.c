#include "header.h"

void generate_random_string(int length, char *output);
int get_token_key(char *key);
void decode_result_to_str(enum l8w8jwt_validation_result result, char *output);

#define DEFAULT_KEY "123456"
#define TOKEN_ISS "NetDisk"
#define TOKEN_EXP (600)

char token_key[128] = DEFAULT_KEY;

struct crypt_data
{
  char output[CRYPT_OUTPUT_SIZE];
  char setting[CRYPT_OUTPUT_SIZE];
  char phrase[CRYPT_MAX_PASSPHRASE_SIZE];
  char initialized;
};

/// @brief 加密密码
/// @param password 明文密码
/// @param encrypted 用于返回加密后的密码
void encrypt_password(const char *password, char *encrypted)
{
  struct crypt_data data;
  bzero(&data, sizeof(data));
  char rand_str[25];
  generate_random_string(24, rand_str);
  crypt_gensalt_rn("$2b$", 11, rand_str, strlen(rand_str), data.setting, CRYPT_OUTPUT_SIZE);
  char *hash = crypt_r(password, data.setting, &data);
  strcpy(encrypted, hash);
}

/// @brief 比较密码
/// @param password 明文密码
/// @param encrypted 加密后的密码
/// @return 密码是否匹配
bool match_password(const char *password, const char *encrypted)
{
  struct crypt_data data;
  bzero(&data, sizeof(data));
  // struct timeval begin, end;
  // gettimeofday(&begin, NULL);
  crypt_r(password, encrypted, &data);
  // gettimeofday(&end, NULL);
  // unsigned long elapsed = (end.tv_sec - begin.tv_sec) * 1000000LL + (end.tv_usec - begin.tv_usec);
  // printf("the time diff si %lu\n", elapsed / 1000);
  return strcmp(data.output, encrypted) == 0;
}

// 生成随机字符串
void generate_random_string(int length, char *output)
{
  static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_><?@#$^&*()";

  if (length <= 0)
  {
    return;
  }

  srand(time(NULL)); // 设置随机种子

  for (int i = 0; i < length; i++)
  {
    output[i] = charset[rand() % (sizeof(charset) - 1)];
  }
  output[length] = '\0'; // 添加字符串结束符
}

#define BUFFER_SIZE (4096 * 1024)

// 将字符串转换为十六进制字符串
void string_to_hex(const unsigned char *input, int length, char *output);

/// @brief 生成文件摘要, 文件游标自动重置到起始位置，函数返回并不会关闭文件描述符，同时游标位置处于文件末尾
/// @param fd 文件描述符
/// @param output 结果字符串
void generate_digest(int fd, char *output)
{
  lseek(fd, 0, SEEK_SET);
  unsigned char buffer[BUFFER_SIZE];
  ssize_t bytesRead;
  SHA256_CTX sha256Context;
  unsigned char hash[SHA256_DIGEST_LENGTH];

  // 初始化SHA-256上下文
  SHA256_Init(&sha256Context);

  // 逐块读取文件并更新哈希值
  while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0)
  {
    SHA256_Update(&sha256Context, buffer, bytesRead);
  }

  // 计算最终哈希值
  SHA256_Final(hash, &sha256Context);

  // 将哈希值转换为十六进制字符串
  string_to_hex(hash, SHA256_DIGEST_LENGTH, output);
}

/// @brief 将hash值转换为十六进制摘要字符串
/// @param input hash 值
/// @param length hash值长度
/// @param output 结果返回指针
void string_to_hex(const unsigned char *input, int length, char *output)
{
  if (!output)
  {
    return;
  }

  for (size_t i = 0; i < length; i++)
  {
    sprintf(output + i * 2, "%02X", input[i]);
  }
  output[length * 2] = '\0'; // 添加字符串结束符
}

/// @brief 判断文件是否和给定的摘要匹配, 文件游标位置自动定位到文件开始，函数返回并不会关闭文件描述符，同时游标位置处于文件末尾
/// @param fd 文件描述符
/// @param digest 目标摘要
/// @return 匹配返回 true，否则返回 false
bool match_digest(int fd, char *digest)
{
  lseek(fd, 0, SEEK_SET);
  char inputDigest[DIGEST_LEN];
  generate_digest(fd, inputDigest);
  printf("inputDigest: %s\n", inputDigest);
  return strcmp(digest, inputDigest) == 0;
}

int generate_token(token_info *info, char *JWT)
{
  size_t jwt_length;
  char KEY[128] = {0};
  get_token_key(KEY);
  struct l8w8jwt_claim payload_claims[] =
      {
          {.key = "workpath",
           .key_length = 8,
           .value = info->workpath,
           .value_length = strlen(info->workpath),
           .type = L8W8JWT_CLAIM_TYPE_STRING},
      };

  struct l8w8jwt_encoding_params params;
  l8w8jwt_encoding_params_init(&params);

  params.alg = L8W8JWT_ALG_HS512;
  char user_id[65] = {0};
  sprintf(user_id, "%ld", info->user_id);
  params.sub = user_id;
  params.sub_length = strlen(user_id);

  params.iss = TOKEN_ISS;
  params.iss_length = strlen(TOKEN_ISS);

  params.iat = l8w8jwt_time(NULL);
  params.exp = l8w8jwt_time(NULL) + TOKEN_EXP; // Set to expire after 10 minutes (600 seconds).

  params.additional_payload_claims = payload_claims;
  params.additional_payload_claims_count = sizeof(payload_claims) / sizeof(struct l8w8jwt_claim);

  params.secret_key = (unsigned char *)KEY;
  params.secret_key_length = strlen(KEY);
  char *temp_jwt = (char *)malloc(TOKEN_LEN);
  params.out = &temp_jwt;
  params.out_length = &jwt_length;

  int result = l8w8jwt_encode(&params);
  if (result != L8W8JWT_SUCCESS)
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "l8w8jwt_encode_hs512 encode failed");
    free(temp_jwt);
    return -1;
  }
  if (jwt_length > TOKEN_LEN)
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "生成的token长度为:%d , 超过了系统设置的最大长度:%d", jwt_length, TOKEN_LEN);
    free(temp_jwt);
    return -1;
  }
  strcpy(JWT, temp_jwt);
  free(temp_jwt);
  LOG(LOG_LEVEL_DEBUG, "TOKEN  :\n %s", JWT);
  return 0;
}

/// @brief 验证token是否有效并获取用户信息
/// @param JWT 待验证的token
/// @param info 用户信息存储结构体
/// @return 成功0， 失败非0 可能是 -1 1 2 4 8 16 32 64 128
int verify_token(const char *JWT, token_info *info)
{
  // 获取token_key
  char KEY[128] = {0};
  get_token_key(KEY);

  struct l8w8jwt_decoding_params params;
  l8w8jwt_decoding_params_init(&params);

  params.alg = L8W8JWT_ALG_HS512;

  params.jwt = (char *)JWT;
  params.jwt_length = strlen(JWT);
  params.verification_key = (unsigned char *)KEY;
  params.verification_key_length = strlen(KEY);

  struct l8w8jwt_claim *claims = NULL;
  size_t claim_count = 0;

  enum l8w8jwt_validation_result validation_result;
  int result = l8w8jwt_decode(&params, &validation_result, &claims, &claim_count);

  // 验证decode结果
  if (result != L8W8JWT_SUCCESS)
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "l8w8jwt_decode_hs512 failed");
    return -1;
  }
  if (validation_result != L8W8JWT_VALID)
  {
    char err_msg[128];
    decode_result_to_str(validation_result, err_msg);
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "token validation failed : %s", err_msg);
    return validation_result;
  }

  // 遍历claims，获取用户信息
  for (size_t i = 0; i < claim_count; i++)
  {
    LOG(LOG_LEVEL_DEBUG, "Claim [%zu]: %s = %s\n", i, claims[i].key, claims[i].value);
    if (strcmp(claims[i].key, "workpath") == 0)
    {
      strcpy(info->workpath, claims[i].value);
    }
    else if (strcmp(claims[i].key, "sub") == 0)
    {
      info->user_id = atol(claims[i].value);
    }
  }
  return validation_result;
}

/// @brief 获取token_key，如果配置文件没有设置则生成一个随机字符串
/// @param key 结果返回指针
/// @return 成功返回0，失败返回-1
int get_token_key(char *key)
{
  if (strncmp(token_key, DEFAULT_KEY, strlen(token_key)) == 0)
  {
    char *config_key = read_config("token_key");
    if (config_key == NULL)
    {
      config_key = (char *)malloc(21);
      generate_random_string(20, config_key);
      LOG(LOG_LEVEL_DEBUG, "token key 使用随机生成的字符串[token_key]:%s", config_key);
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "token key 使用配置文件中的[token_key]:%s", config_key);
    }
    strcpy(token_key, config_key);
    free(config_key);
  }
  strcpy(key, token_key);
  return 0;
}

/// @brief verify_token的结果转换为字符串
/// @param result 验证结果
/// @param output 输出字符串地址
void decode_result_to_str(enum l8w8jwt_validation_result result, char *output)
{
  switch (result)
  {
  case L8W8JWT_VALID:
    strcpy(output, "L8W8JWT_VALID");
    break;
  case L8W8JWT_ISS_FAILURE:
    strcpy(output, "The issuer claim is invalid.");
    break;
  case L8W8JWT_SUB_FAILURE:
    strcpy(output, "The subject claim is invalid.");
    break;
  case L8W8JWT_AUD_FAILURE:
    strcpy(output, "The audience claim is invalid.");
    break;
  case L8W8JWT_JTI_FAILURE:
    strcpy(output, "The JWT ID claim is invalid.");
    break;
  case L8W8JWT_EXP_FAILURE:
    strcpy(output, "The token is expired.");
    break;
  case L8W8JWT_NBF_FAILURE:
    strcpy(output, "The token is not yet valid.");
    break;
  case L8W8JWT_IAT_FAILURE:
    strcpy(output, "The token was not issued yet, are you from the future?");
    break;
  case L8W8JWT_SIGNATURE_VERIFICATION_FAILURE:
    strcpy(output, "The token was potentially tampered with: its signature couldn't be verified.");
    break;
  case L8W8JWT_TYP_FAILURE:
    strcpy(output, "The token type claim is invalid.");
    break;
  default:
    strcpy(output, "UNKNOWN");
    break;
  }
}