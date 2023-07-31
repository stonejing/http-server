#pragma once

#include <string>
#include <iostream>
#include <map>
#include <fstream>

using namespace std;

const string ok_200_title = "OK";
const string error_400_title = "Bad Request";
const string error_400_content = "Your request has bad syntax";
const string error_403_title = "Forbidden";
const string error_403_content = "You do not have permission to get file";
const string error_404_title = "Not Found";
const string error_404_content = "The requested file was not found";
const string error_500_title = "Internal Error";
const string error_500_content = "There was an unusual problem serving the requested file.";

const string server_header = "Server: stonejing webserver\r\n";
// const string type_header = "Content-Type: text/html\r\n";
const string type_header = "";
const string headers = server_header + type_header;

class HttpResponse
{
public:
    HttpResponse()
    {
        status_line_map_[200] = ok_200_title;
        status_line_map_[400] = error_400_title;
        status_line_map_[403] = error_403_title;
        status_line_map_[404] = error_404_title;
        status_line_map_[500] = error_500_title;
    }

    string& get_response(int& size) 
    { 
        set_response();
        size = response_.size(); 
        return response_; 
    }

    void set_information(bool keep_alive, string& URL)
    {
        keep_alive_ = keep_alive;
        URL_ = URL;
    }

private:
    void set_response()
    {
        response_.clear();
        response_ += "HTTP/1.1 ";
        get_content();
        add_status_line();
        add_response_header();
        add_content();
    }

    void add_status_line()
    {
        response_ = response_ + std::to_string(status_) + " " + status_line_map_[status_] + "\r\n";
    }

    void add_linger()
    {
        response_ = response_ + "Connection: " + (keep_alive_ ? "keep-alive\r\n" : "close\r\n");
    }

    void add_blank_line() { response_ += "\r\n"; }

    void add_content_length() { response_ += "Content-Length: " + to_string(content_.size()) + "\r\n"; }

    void add_response_header()
    {
        response_ += headers;
        add_linger();
        add_content_length();
        add_blank_line();
    }

    void add_content()
    {
        response_ += content_;
    }

    void get_content() 
    {
        cout << "get_content: " << URL_ << endl;
        if(URL_.empty())
        {
            status_ = 400;
            content_ = "resource not found";
            return;
        }
        if(URL_ == "/")
        {
            status_ = 200;
            content_ = "HELLO WORLD";
            return;
        }
        string file_path = root_path_ + URL_;
        std::ifstream file(file_path);
        if(!file.is_open())
        {
            std::cerr << "Error opening file." << std::endl;
            return;
        }
        content_.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
    }

private:
    std::map<int, string> status_line_map_{};

    // only reponse will be accessed.
    string response_{};      // response that should be returned

    int status_ = 200;
    bool keep_alive_ = true;     // linger, keep alive
    const string root_path_{"/mnt/d/Code/project/http-server/resource"};
    string URL_{};           // resource file path
    string content_{};       // response entity content  
};