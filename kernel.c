void _start() {
    volatile char* uart = (volatile char*)0x09000000;
    uart[0] = 'H';
    uart[0] = 'e';
    uart[0] = 'l';
    uart[0] = 'l';
    uart[0] = 'o';
    uart[0] = '\r';
    uart[0] = '\n';
}