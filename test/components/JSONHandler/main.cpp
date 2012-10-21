#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <algorithm>
//#include <cctype>

#include "JSONHandler/MobileRPCMessage.h"
#include "ProtocolHandler.hpp"
#include "CBTAdapter.hpp"
#include <vector>

#include "TestJSONHandler.h"

char * readJsonContent ( const char * fileName );
int parseJson ( const std::string & jsonContent );
int withBluetooth();

int main( int argc, char* argv[] ) {
    
    char * fileName = "/home/dev/Projects/Ford/source/workspace/jsonSample/connection_log_json_only" ;    
    if ( argc > 1 )
    {
        fileName = argv[1];
    }

    char * jsonContent = readJsonContent( fileName );

    //withBluetooth();

    int result = 0;
    size_t nextMessagePos = 0;
    char *const jsonContentBeginning = jsonContent;

    /*AxisCore::CMessage::generateInitialMessage();*/
    JSONHandler * jsonHandler = new JSONHandler;
    TestJSONHandler testJSONHandler;
    testJSONHandler.jsonHandler = jsonHandler;
    /*AxisCore::ProtocolHandler * protocolHandler = new AxisCore::ProtocolHandler( jsonHandler, 0 );
    jsonHandler -> setProtocolHandler( protocolHandler );*/
    char * secondname = "/home/dev/Projects/Ford/source/workspace/jsonSample/jsonrpc2_sample";
    char * rpc2Content = readJsonContent( secondname );
    testJSONHandler.RPC2( rpc2Content );
    
    while ( nextMessagePos != std::string::npos ) {

        std::string jsonString( jsonContent );
        if ( !jsonString.empty() ) {
            testJSONHandler.secondRelease( jsonString );
        }

        nextMessagePos = jsonString.find( "\n" );
        jsonContent += nextMessagePos + 1;

        sleep( 10 );
    }

    delete [] jsonContentBeginning;
    delete jsonHandler;
    //delete protocolHandler;

    return result;
}

char *
readJsonContent ( const char * fileName ) {
    using namespace std;

    ifstream file_str;
    file_str.open ( fileName );

    file_str.seekg( 0, ios::end );
    int length = file_str.tellg();
    file_str.seekg( 0, ios::beg );

    char * raw_data = new char[length];
    file_str.read( raw_data, length );
    file_str.close();

    char * input = new char[length];
    for ( int i = 0; i < length; ++i )
    {
        if ( !isspace(raw_data[i]) ) 
        {
            input[i] = raw_data[i];
        }
    }
    /*//char* newEnd = std::remove_if( raw_data, raw_data + length, std::isspace );*/
    /*std::string result ( raw_data, length - 1 );
    std::string::iterator it = std::remove_if( result.begin(), result.end(), std::isspace );*/
    
    //std::string result ( raw_data, newEnd - 1 );



    return raw_data;
}

int
parseJson ( const std::string & jsonContent ) {
    Json::Value root;   // will contains the root value after parsing.
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( jsonContent, root );
    if ( !parsingSuccessful ) {
        std::cout  << "Failed to parse configuration\n"
                   << reader.getFormatedErrorMessages();
        return -1;
    }

    // Get the value of the member of root named 'encoding', return 'UTF-8' if there is no
    // such member.
    //Json::Value val = root.get("encoding", Json::Value::null);
    Json::Value val = root["request"];
    if ( val.isNull() ) {
        std::cout<<"null"<<std::endl;
    } else if ( val.isArray() ) {
        //for ( int i = 0; i < val.size(); ++i ) {
            Json::Value x = val["correlationID"];
            if ( x.isNull() ) {
                std::cout << "no correlationID" << std::endl;
            }
        //}
    } else {
        Json::Value x = val["correlationID"];
        if ( x.isNull() ) {
            std::cout << "no correlationID" << std::endl;
        } else {
            std::cout << x.asInt() << std::endl;
        }
        std::cout << "smth" << std::endl;
    }
    //.asString();
    //std::cout << encoding << std::endl;

    Json::StyledWriter writer;
    std::string root_to_print = writer.write( root );
    std::cout << root_to_print;
    return 0;
}

