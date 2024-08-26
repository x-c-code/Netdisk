#include "head.h"
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


