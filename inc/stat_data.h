#include <string>
#include <map>
#include <fstream>
using namespace std;
/*!
   统计数据类 分装数据的读取和保存
 */
class StatData 
{
private:
    map<string, int> stat_data;
    string filepath;

public:
    StatData(string filepath);
    // 统计+1，返回统计后的数字
    int incr(string key);
    void save();
    // 返回统计数据 行结束 可以是 \n <br> ...
    string get(string endline = "\n");
};


