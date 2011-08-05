gcc packetgenerator.c -o packetgenerator -I./include ${1}
gcc porca.c -o porca -lpthread -I./include ${1}

