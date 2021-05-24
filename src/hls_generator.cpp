#include <exception>
#include <iostream>
#include <stdexcept>
#include "hls_generator.h"

void Hls_generator::add_output(const Profile& profile)
{
    outputs.push_back(profile);
}
void Hls_generator::start()
{
    if(input_url.empty()) 
        throw std::runtime_error("Empty input url");

    if(outputs.empty()) 
        throw std::runtime_error("Empty output profile");
    
}
Hls_generator::~Hls_generator(){
    std::cout << "End Hls_generator\n";
}
