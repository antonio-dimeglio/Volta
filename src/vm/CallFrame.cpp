#include "vm/CallFrame.hpp"

namespace volta::vm {

CallFrame::CallFrame(
    ClosureObject* closure,
    uint8_t* return_addr,
    uint8_t return_reg
) {
    std::fill(std::begin(registers), std::end(registers), Value::none());
    return_address = return_addr;
    return_register = return_register;
    this->closure = closure; 
    if (closure && closure->function) {
        function_name = closure->function->name;
    } else {
        function_name = "<unknown>";
    }
    line_number = 0;
}

} // namespace volta::vm
