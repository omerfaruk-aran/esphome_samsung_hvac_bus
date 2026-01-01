g++ -std=c++17 $1 components/samsung_ac/protocol.cpp components/samsung_ac/protocol_nasa.cpp components/samsung_ac/protocol_non_nasa.cpp components/samsung_ac/util.cpp components/samsung_ac/debug_mqtt.cpp components/samsung_ac/log.cpp -Itest -o test.exe
chmod +x test.exe
./test.exe
