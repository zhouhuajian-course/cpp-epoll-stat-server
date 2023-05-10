#include "stat_data.h"
using namespace std;

string StatData::get(string endline) {
    string statData;
    for(auto item : stat_data) {
        statData += item.first + "=" + to_string(item.second) + endline;
    }
    return statData;
}
void StatData::save() 
{
    // 输出文件流对象保存数据
    ofstream ofs(this->filepath);
    string statData = get();
    ofs.write(statData.c_str(), statData.length());
    ofs.close();
}
int StatData::incr(string key) 
{
    if (stat_data.find(key) == stat_data.end()) {
        stat_data[key] = 1;
        return 1;
    }
    else 
    {
        // 忽略两语句可合并
        stat_data[key]++;
        return stat_data[key];
    }
}
StatData::StatData(string filepath) 
{
    this->filepath = filepath;
    // 输入文件流对象读取文件 文件不存在不会报错
    ifstream ifs(filepath);
    string line;
    // 统计数据
    while (getline(ifs, line)){
        // cout << line << endl;
        int lastColonIndex = line.rfind("=");
        // cout << line.substr(0, lastColonIndex) << "=" << atoi(line.substr(lastColonIndex + 1).c_str())  << endl;
        stat_data[line.substr(0, lastColonIndex)] = atoi(line.substr(lastColonIndex + 1).c_str());
    }
    ifs.close();
}


