#include "httpresponse.h"

string HttpResponse::root_path_ = std::filesystem::current_path().string() + "/../../../resource";

void HttpResponse::get_content()
{
    if(URL_.empty())
    {
        status_ = 400;
        content_ = "resource not found";
        // URL_ = "/404.html";
        return;
    }
    else if(URL_ == "/")
    {
        status_ = 200;
        URL_ = "/index.html";
    }
    else if(URL_ == "/time")
    {
        // Get the current time using <chrono>
        auto currentTime = std::chrono::system_clock::now();
        std::time_t currentTimeT = std::chrono::system_clock::to_time_t(currentTime);
        // Format the time as a string
        std::stringstream timeStream;
        timeStream << std::put_time(std::localtime(&currentTimeT), "%Y-%m-%d %H:%M:%S");
        // Get the formatted time string
        std::string timeString = timeStream.str();
        // Display the current time
        status_ = 200;
        content_ = timeString;
        return;
    }
    else if(URL_ == "/file")
    {
        string current_path = root_path_ + URL_;
        content_ = "";
        for (const auto& entry : std::filesystem::directory_iterator(current_path)) {
            content_ +=  entry.path().filename().string() + "\n";
        }
        status_ = 200;
        return;
    }
    string file_path = root_path_ + URL_;
    std::ifstream file(file_path);
    if(!file.good()) 
    {
        std::ifstream file_404(root_path_ + "/404.html");
        status_ = 404;
        content_.assign(std::istreambuf_iterator<char>(file_404), std::istreambuf_iterator<char>());
        return;
    }
    if(!file.is_open())
    {
        std::cerr << "Error opening file." << std::endl;
        return;
    }
    content_.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
}