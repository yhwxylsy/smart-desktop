#pragma once
#include <Arduino.h>

// 配置状态与 NVS 读写（原 main.ino L32-37、L67-77、L79-130）。
// 去耦合：收归 configStore 命名空间访问器。NVS handle 与内存态全部私有化，
// 外部仅能经下列访问接口读写，禁止直接引用内部全局量。
// 原 CFG:RESET 语义（清 NVS + 复位内存态、不触碰 serverPort）由 reset() 完整保留。
namespace configStore {

// 生命周期
void load();
void save();
void reset();
bool parseServerUrl(String value);

// 只读访问器（返回内部态引用，调用方不得长期持有指针跨生命周期使用）
const String &host();
uint16_t port();
bool secure();
const String &deviceToken();
const String &wifiSsid();
const String &wifiPassword();

// 派生 URL
String httpBase();
String wsBase();
String wsPath();

// 写入接口
void setWifi(const String &ssid, const String &password);
void setToken(const String &token);

}  // namespace configStore
