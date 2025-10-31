import socket
import redis
from typing import List, Tuple, Any

class RESPInterceptor:
    """Intercepts and shows RESP protocol messages"""
    
    def __init__(self, host='localhost', port=6379):
        self.host = host
        self.port = port
    
    def encode_resp_array(self, items: List[str]) -> str:
        """Encode a list of strings as RESP array"""
        result = f"*{len(items)}\r\n"
        for item in items:
            result += f"${len(item)}\r\n{item}\r\n"
        return result
    
    def decode_resp_response(self, data: bytes) -> str:
        """Decode RESP response to readable format"""
        if not data:
            return ""
        
        response = data.decode('utf-8', errors='ignore')
        
        # Format for display
        formatted = ""
        lines = response.split('\r\n')
        
        for line in lines:
            if not line:
                formatted += "\\r\\n\n"
            elif line.startswith('+'):
                formatted += f"+ Simple String: '{line[1:]}'\n"
            elif line.startswith('-'):
                formatted += f"- Error: '{line[1:]}'\n"
            elif line.startswith(':'):
                formatted += f": Integer: {line[1:]}\n"
            elif line.startswith('$'):
                length = line[1:]
                if length == '-1':
                    formatted += "$ Bulk String: (nil)\n"
                else:
                    formatted += f"$ Bulk String: length {length}\n"
            elif line.startswith('*'):
                count = line[1:]
                if count == '-1':
                    formatted += "* Array: (nil)\n"
                else:
                    formatted += f"* Array: {count} elements\n"
            else:
                # Data line
                if line and not line.startswith(('*', '$', ':', '+', '-')):
                    formatted += f"  Data: '{line}'\n"
        
        return formatted.strip()
    
    def _parse_result_from_response(self, data: bytes) -> Any:
        """Parse the actual result from RESP response data"""
        if not data:
            return None
        
        response = data.decode('utf-8', errors='ignore')
        lines = response.split('\r\n')
        
        if not lines:
            return None
        
        first_line = lines[0]
        
        # Handle different RESP types
        if first_line.startswith('+'):
            # Simple string
            return first_line[1:]
        elif first_line.startswith('-'):
            # Error
            return f"ERROR: {first_line[1:]}"
        elif first_line.startswith(':'):
            # Integer
            try:
                return int(first_line[1:])
            except ValueError:
                return first_line[1:]
        elif first_line.startswith('$'):
            # Bulk string
            length = first_line[1:]
            if length == '-1':
                return None  # nil
            else:
                try:
                    length_val = int(length)
                    if len(lines) > 1:
                        return lines[1][:length_val] if len(lines[1]) >= length_val else lines[1]
                except (ValueError, IndexError):
                    return None
        elif first_line.startswith('*'):
            # Array
            count = first_line[1:]
            if count == '-1':
                return None  # nil array
            else:
                try:
                    count_val = int(count)
                    if count_val == 0:
                        return []
                    
                    # Parse array elements (simplified)
                    result = []
                    line_idx = 1
                    for _ in range(count_val):
                        if line_idx < len(lines) and lines[line_idx].startswith('$'):
                            # Bulk string element
                            elem_length = int(lines[line_idx][1:])
                            line_idx += 1
                            if line_idx < len(lines):
                                result.append(lines[line_idx][:elem_length] if len(lines[line_idx]) >= elem_length else lines[line_idx])
                            line_idx += 1
                        elif line_idx < len(lines):
                            # Other types (simplified)
                            result.append(lines[line_idx])
                            line_idx += 1
                    
                    return result
                except (ValueError, IndexError):
                    return []
        
        return response
    
    def execute_command(self, command_parts: List[str]) -> Tuple[str, str, Any]:
        """Execute command and return RESP request, response, and result"""
        try:
            # Create RESP request
            resp_request = self.encode_resp_array(command_parts)
            
            # Format request for display
            formatted_request = ""
            lines = resp_request.split('\r\n')
            for line in lines:
                if not line:
                    formatted_request += "\\r\\n\n"
                elif line.startswith('*'):
                    formatted_request += f"* Array: {line[1:]} elements\n"
                elif line.startswith('$'):
                    formatted_request += f"$ Bulk String: length {line[1:]}\n"
                else:
                    formatted_request += f"  Data: '{line}'\n"
            
            # Execute command using raw socket to capture response
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.host, self.port))
            
            # Send RESP request
            sock.send(resp_request.encode())
            
            # Receive response
            response_data = sock.recv(4096)
            sock.close()
            
            # Format response for display
            formatted_response = self.decode_resp_response(response_data)
            
            # Parse the result from the raw response data for easier handling
            # Instead of executing the command twice, we'll parse the result from the raw response
            result = self._parse_result_from_response(response_data)
            
            return formatted_request.strip(), formatted_response, result
            
        except Exception as e:
            return f"Error creating request: {str(e)}", f"Error: {str(e)}", str(e)