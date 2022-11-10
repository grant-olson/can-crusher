import serial

class SerialException(Exception):
  pass

class SerialCLI:
  ERRORS = {
    0: "ERR_OK",
    100: "ERR_TOO_MANY_ARGS",
    101: "ERR_TOO_FEW_ARGS",
    102: "ERR_UNKNOWN_CMD",
    103: "ERR_EXECUTION_FAILED",
    104: "ERR_OUT_OF_BOUNDS",
    105: "ERR_MOTORS_SLEEPING",
    106: "ERR_NO_12V_POWER",
    107: "ERR_STALL_LEFT",
    108: "ERR_STALL_RIGHT",
    109: "ERR_STALL_BOTH",
    110: "ERR_STALL_UNKNOWN",
    111: "ERR_BAD_PROP_NAME"
    
    }
  
  COMMANDS = [
    
    "ECHO",
    "HOME",
    "MOVE",
    "MOVE_LEFT",
    "MOVE_RIGHT",
    "POSITION?",
    "POWER_OFF",
    "POWER_ON",
    "PROP=",
    "PROP?",
    "SLEEP",
    "VERSION?",
    "WAKE",
    "ZERO",
    ]
  
  def __init__(self, device):
    self.device = device
    self.serial_conn = serial.Serial(device, 115200)

  def get_error_code(self, result):
    if result == 'OK\r\n':
      return None
    elif result[0:3] == "ERR":
      return int(result.strip().split(" ")[-1])
    else:
      raise RuntimeError("NOT A STATUS CODE")

  def cmd(self, *args):
    args = [str(x) for x in args]
    full_cmd = " ".join(args)
    full_cmd += "\n"
    full_cmd = bytes(full_cmd, "utf-8")
    
    self.serial_conn.write(full_cmd)
    self.serial_conn.readline() # Same command echoed back.

    answer = None
    error_code = None

    result = self.serial_conn.readline().decode("utf-8")

    if ":" in result:
      answer = result.strip().split(": ")[-1]
      result = self.serial_conn.readline().decode("utf-8")
    
    error_code = self.get_error_code(result)

    if error_code:
      raise SerialException(SerialCLI.ERRORS[error_code])
      
    return answer

  def version(self):
    return self.cmd("VERSION?")
  
  def __getattr__(self, attr_name):
    if attr_name == "set_prop":
      attr_name = "PROP="
    elif attr_name == "get_prop":
      attr_name = "PROP?"
      
    attr_name = attr_name.upper()
    if attr_name in SerialCLI.COMMANDS:
      def wrapped_method(*args):
        return self.cmd(attr_name, *args)
      return wrapped_method
    else:
      raise AttributeError()
