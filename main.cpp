#include "main.h"

MyClientClass* client;
std::unordered_map<std::string, void*> fptr_map;
std::unordered_map<std::string, void*> module_map;

void MyClientClass::onMessage(SleepyDiscord::Message message) {
    if(message.startsWith("!loadModule")){
        std::string contents = message.content;
        std::string module_name = contents.substr(contents.find(" ")+1);
        if(module_map.count(module_name)){
            sendMessage(message.channelID, "Module already loaded");
            return;
        }
        void* handle = loadModule((char*)module_name.c_str());
        if(handle == NULL){
            sendMessage(message.channelID, "Module failed to load");
            return;
        }
        module_map[module_name] = handle;
        sendMessage(message.channelID, "Module loaded");
        return;
    }
    if(message.startsWith("!unloadModule")){
        std::string contents = message.content;
        std::string module_name = contents.substr(contents.find(" ")+1);
        if(!module_map.count(module_name)){
            sendMessage(message.channelID, "Module not loaded");
            return;
        }
        int ret = unloadModule((char*)module_name.c_str());
        if(ret){
            sendMessage(message.channelID, "Module failed to unload");
            return;
        }
        module_map.erase(module_name);
        sendMessage(message.channelID, "Module unloaded");
        return;
    }
    if(message.startsWith("!")){
        std::string contents = message.content;
        std::string command = contents.substr(1, contents.find(" "));
        std::string args = contents.substr(contents.find(" ")+1);
    
        void (*fptr)(SleepyDiscord::Message, char*) = (void (*)(SleepyDiscord::Message, char*)) fptr_map.at(command);
        char* arg = (char*)malloc(sizeof(char) * contents.size()+1);
        strcpy(arg, (char*)contents.c_str());
        fptr(message, arg);
        free(arg);
    }
}

int main(int argc, char** argv){
    //load configurations 
    config_t cfg;
    config_init(&cfg);
    if(! config_read_file(&cfg, "discord.cfg")) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
        config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return(EXIT_FAILURE);
    }
    const char* token;
    if(!config_lookup_string(&cfg, "token", &token)) {
        fprintf(stderr, "No 'token' setting in configuration file.\n");
        return(EXIT_FAILURE);
    }
    
    //initialize bot
    MyClientClass bot(token, SleepyDiscord::USER_CONTROLED_THREADS);
    client = &bot;
    client->run();
}

void* loadModule(char* module_name){
    char path[512] = "modules/";
    strcat(path, module_name);
    strcat(path, ".so");
    void* handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    std::string m_name = module_name;
    if(handle != NULL)
        module_map[m_name] = handle;
    return handle;
}

int unloadModule(char* module_name) {
    std::string m_name = module_name;
    int retval = dlclose(module_map.at(m_name));
    return retval;
}
