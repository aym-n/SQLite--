#include<iostream>
#include<string>


using namespace std;

class InputBuffer {
    public:

    string buffer;
    size_t buffer_length;
    ssize_t input_buffer;

    InputBuffer() {
        input_buffer = 0;
        buffer_length = 0;
    }
};
void print_prompt () {
    cout  <<  " db > ";
}
void read_input (InputBuffer * input_buffer) {
    getline(cin, input_buffer -> buffer );
    input_buffer -> buffer_length = input_buffer -> buffer.length();
    if(!input_buffer->buffer.empty() && input_buffer->buffer[input_buffer->buffer_length - 1] == '\n'){
        input_buffer -> buffer.pop_back(); //new line character remove kiya as it may effect the comparision process
        input_buffer -> buffer_length--; // as i am removing the last character so length kam hogi
    }
}

int main(){
    InputBuffer input_buffer;
    while(true) {
        print_prompt();
        read_input( &input_buffer );
        if(input_buffer.buffer == ".exit"){
            exit(EXIT_SUCCESS);
        }
        else{
            cout << "unrecognised command " << input_buffer.buffer;
        }
    }
    return 0;
}