#include <iostream>
#include <string>
#include <unistd.h>
#include "rtm_client.h"

using namespace std;
const string APP_ID = "Your Agora APPID";
const string account = "agora.pi";

int main(int argc, const char * argv[]) {
  DemoMessaging messaging(APP_ID, account);
  messaging.run();
  return 0;
}