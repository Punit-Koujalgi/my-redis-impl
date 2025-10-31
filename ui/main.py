import gradio as gr
import redis
import subprocess
import time
import shlex
from typing import List, Tuple, Optional
import os
import sys

# Add the ui directory to path for imports
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from redis_commands import REDIS_COMMANDS
from resp_interceptor import RESPInterceptor

class RedisPlayground:
	def __init__(self):
		self.server_process = None
		self.redis_client = None
		self.resp_interceptor = None
		
	def start_redis_server(self):
		"""Start the Redis server in the background"""

		if self.server_process:
			return False, "✅ Redis server is already running!"
		
		try:
			# Change to the project root directory
			project_root = os.path.dirname(os.path.abspath(__file__))
			server_path = os.path.join(project_root, "server")
			
			if not os.path.exists(server_path):
				return False, "❌ Server binary not found. Please run 'cmake --build build' first."
			
			# Ensure the server binary has execute permissions
			import stat
			current_permissions = os.stat(server_path).st_mode
			os.chmod(server_path, current_permissions | stat.S_IEXEC)
			
			# Start the server
			self.server_process = subprocess.Popen(
				[server_path],
				cwd=project_root,
				stdout=subprocess.PIPE,
				stderr=subprocess.PIPE
			)

			# Give server time to start
			time.sleep(2)
			
			# Test connection
			try:
				self.redis_client = redis.Redis(host='localhost', port=6379, decode_responses=True)
				self.resp_interceptor = RESPInterceptor(host='localhost', port=6379)
				self.redis_client.ping()
				return True, "✅ Redis server started successfully!"
			except Exception as e:
				return False, f"❌ Failed to connect to Redis server: {str(e)}"
				
		except Exception as e:
			return False, f"❌ Failed to start Redis server: {str(e)}"
	
	def stop_redis_server(self):
		"""Stop the Redis server"""
		if self.server_process:
			self.server_process.terminate()
			self.server_process.wait()
			self.server_process = None
		if self.redis_client:
			self.redis_client.close()
			self.redis_client = None
	
	def parse_command(self, command: str) -> List[str]:
		"""Parse a Redis command handling quotes properly"""
		try:
			# Use shlex to properly parse quoted strings
			return shlex.split(command)
		except ValueError:
			# If shlex fails, fall back to simple split
			return command.strip().split()
	
	def execute_command(self, command: str) -> Tuple[str, str, str]:
		"""Execute Redis command(s) and return output, RESP request, and RESP response"""
		if not self.redis_client:
			return "❌ Redis server not connected", "", ""
		
		# Handle multiple commands separated by newlines
		commands = [cmd.strip() for cmd in command.split('\n') if cmd.strip() and cmd.startswith('#') is False]
		if not commands:
			return "❌ Empty command", "", ""
		
		all_outputs = []
		all_requests = []
		all_responses = []
		
		for i, cmd in enumerate(commands):
			try:
				# Parse command with proper quote handling
				parts = self.parse_command(cmd)
				if not parts:
					continue
				
				# Get RESP format before execution
				resp_request, resp_response, result = self.resp_interceptor.execute_command(parts) #type: ignore
				
				# Format the output
				if isinstance(result, (list, tuple)):
					if all(isinstance(item, str) for item in result):
						output = "\n".join(f"{j+1}) {item}" for j, item in enumerate(result))
					else:
						output = str(result)
				elif result is None:
					output = "(nil)"
				elif isinstance(result, bool):
					output = "OK" if result else "(error)"
				else:
					output = str(result)
				
				# Add command number prefix for multiple commands
				cmd_prefix = f"Command {i+1}: {cmd}\n" if len(commands) > 1 else ""
				all_outputs.append(f"{cmd_prefix}✅ {output}")
				
				# Add separator for RESP sections
				req_prefix = f"=== Command {i+1}: {cmd} ===\n" if len(commands) > 1 else ""
				all_requests.append(f"{req_prefix}{resp_request}")
				all_responses.append(f"{req_prefix}{resp_response}")
				
			except redis.ResponseError as e:
				cmd_prefix = f"Command {i+1}: {cmd}\n" if len(commands) > 1 else ""
				all_outputs.append(f"{cmd_prefix}❌ {str(e)}")
				all_requests.append(f"{cmd_prefix}Error in command")
				all_responses.append(f"{cmd_prefix}Error: {str(e)}")
			except Exception as e:
				cmd_prefix = f"Command {i+1}: {cmd}\n" if len(commands) > 1 else ""
				all_outputs.append(f"{cmd_prefix}❌ Error: {str(e)}")
				all_requests.append(f"{cmd_prefix}Error in command")
				all_responses.append(f"{cmd_prefix}Error: {str(e)}")
		
		return ("\n\n".join(all_outputs), 
				"\n\n".join(all_requests), 
				"\n\n".join(all_responses))

# Initialize the playground
playground = RedisPlayground()

def create_ui():
	"""Create the Gradio UI"""
	
	# Custom CSS for better styling
	css = """
	.command-card {
		border: 1px solid #e1e5e9;
		border-radius: 8px;
		padding: 16px;
		margin: 8px 0;
		background: #f8f9fa;
	}
	.resp-section {
		font-family: 'Courier New', monospace;
		background: #2d3748;
		color: #e2e8f0;
		padding: 12px;
		border-radius: 6px;
		white-space: pre-wrap;
	}
	.terminal {
		font-family: 'Courier New', monospace;
		background: #1a202c;
		color: #00ff00;
		padding: 16px;
		border-radius: 8px;
		min-height: 200px;
	}

	#server-status {
		padding-top: 50px;
	}
	"""
	
	with gr.Blocks(css=css, title="Redis Playground 🚀", theme=gr.themes.Default(), fill_width=True) as demo: #type: ignore
		
		with gr.Row():
			with gr.Column():
				# Header
				gr.Markdown("""
				# 🚀 Redis Playground!
				
				Welcome to the **Redis Playground**! This is an interactive environment to explore Redis commands and understand the RESP (Redis Serialization Protocol) in action.
				
				**What is Redis?** 🤔
				Redis is an in-memory data structure store used as a database, cache, and message broker. It supports various data structures like strings, lists, streams, and powerful features like transactions and pub/sub messaging.
				
				**This Redis Implementation Features:** ✨
				- 🎯 **Complete RESP Protocol** - See raw protocol messages
				- 📝 **Rich Data Types** - Strings, Lists, Streams with time-series data
				- ⚡ **Advanced Features** - Transactions (MULTI/EXEC), Pub/Sub messaging
				- 🔄 **Real-time Operations** - Blocking operations (BLPOP), Stream reads (XREAD)
				- 🏗️ **Production Ready** - Replication, persistence, concurrent clients (you would have to run server following [github repo](https://github.com/Punit-Koujalgi/my-redis-impl) locally to see these features in action)

				""")
				
			# Server status
			with gr.Column(elem_id="server-status"):
				with gr.Row():
					gr.Markdown("""
				**What you can do here:** 🎮
				- 🖥️ Execute commands in a terminal-like interface  
				- 👁️ See the raw RESP protocol messages for learning
				- 🌊 Experiment with Redis Streams for event logging
				- 🔄 Try transactions for atomic operations
				- 📡 Test pub/sub for real-time messaging
				""")
					
				with gr.Row():
					start_btn = gr.Button("🚀 Start Server", variant="primary")
					stop_btn = gr.Button("🛑 Stop Server", variant="stop")
				with gr.Row():
					server_status = gr.Textbox(
						label="🖥️ Server Status",
						value="🔄 Starting Redis server...",
						interactive=False
					)

		# Main content
		with gr.Row():
			# Left column - Commands Explorer
			with gr.Column(scale=1):
				gr.Markdown("## 📚 Commands Explorer")
				
				command_dropdown = gr.Dropdown(
					choices=list(REDIS_COMMANDS.keys()),
					label="🎯 Select a Redis Command",
					value="PING"
				)
				
				command_info = gr.Markdown(value=REDIS_COMMANDS["PING"]["description"], height=200)
				command_example = gr.Code(
					value=REDIS_COMMANDS["PING"]["example"],
					language="shell",
					label="💡 Example Usage"
				)
				
				try_example_btn = gr.Button("🚀 Try This Example", variant="secondary")
			
			# Right column - Terminal Interface
			with gr.Column(scale=1):
				gr.Markdown("## 🖥️ Redis Terminal")
				
				command_input = gr.Textbox(
					label="💻 Enter Redis Command(s)",
					placeholder="Type Redis commands here... (use multiple lines for multiple commands)\nExample: SET mykey myvalue",
					lines=3
				)
				
				execute_btn = gr.Button("⚡ Execute", variant="primary")
				
				# Output sections
				with gr.Tabs():
					with gr.TabItem("📤 Output"):
						command_output = gr.Textbox(
							label="📋 Result",
							lines=8,
							interactive=False,
							elem_classes=["terminal"]
						)
					
					with gr.TabItem("📨 RESP Request"):
						resp_request = gr.Code(
							label="📤 RESP Request Format",
							# language="text",
							lines=6,
							elem_classes=["resp-section"]
						)
					
					with gr.TabItem("📬 RESP Response"):
						resp_response = gr.Code(
							label="📥 RESP Response Format", 
							# language="text",
							lines=6,
							elem_classes=["resp-section"]
						)
		
		# Quick commands section
		gr.Markdown("## ⚡ Quick Commands")
		with gr.Row():
			quick_commands = [
				("PING", "🏓"),
				("SET hello world", "💾"),
				("GET hello", "📖"),
				("KEYS *", "🔑"),
				("INFO replication", "ℹ️"),
				("INCR counter", "➕")
			]
			
			quick_btns = []
			for cmd, emoji in quick_commands:
				btn = gr.Button(f"{emoji} {cmd}", size="sm")
				quick_btns.append((btn, cmd))
		
		# Second row of quick commands for more advanced features
		with gr.Row():
			advanced_commands = [
				("LPUSH tasks task1", "⬅️"),
				("RPUSH queue job1", "➡️"),
				("LRANGE tasks 0 -1", "📋"),
				("TYPE hello", "🏷️"),
				("XADD events * action login", "🌊"),
				("MULTI", "🔄")
			]
			
			advanced_btns = []
			for cmd, emoji in advanced_commands:
				btn = gr.Button(f"{emoji} {cmd}", size="sm")
				advanced_btns.append((btn, cmd))
		
		# Event handlers
		def update_command_info(selected_command):
			if selected_command in REDIS_COMMANDS:
				cmd_info = REDIS_COMMANDS[selected_command]
				return cmd_info["description"], cmd_info["example"]
			return "", ""
		
		def execute_redis_command(command):
			output, req, resp = playground.execute_command(command)
			return output, req, resp
		
		def start_server():
			time.sleep(3)  # Wait a bit for UI to load
			success, message = playground.start_redis_server()
			return message
		
		def stop_server():
			playground.stop_redis_server()
			return "🛑 Redis server stopped"
		
		def set_command(command):
			return command
		
		# Wire up events
		command_dropdown.change(
			update_command_info,
			inputs=[command_dropdown],
			outputs=[command_info, command_example]
		)
		
		execute_btn.click(
			execute_redis_command,
			inputs=[command_input],
			outputs=[command_output, resp_request, resp_response]
		)
		
		command_input.submit(
			execute_redis_command,
			inputs=[command_input],
			outputs=[command_output, resp_request, resp_response]
		)
		
		try_example_btn.click(
			lambda cmd: REDIS_COMMANDS[cmd]["example"].replace('\n# Expected:', '\n# Expected:').replace('\n#', '\n#') if cmd in REDIS_COMMANDS else "",
			inputs=[command_dropdown],
			outputs=[command_input]
		)
		
		start_btn.click(start_server, outputs=[server_status])
		stop_btn.click(stop_server, outputs=[server_status])
		
		# Quick command buttons
		for btn, cmd in quick_btns:
			btn.click(set_command, inputs=[gr.State(cmd)], outputs=[command_input])
		
		for btn, cmd in advanced_btns:
			btn.click(set_command, inputs=[gr.State(cmd)], outputs=[command_input])
		
		# Auto-start server when UI loads
		demo.load(start_server, outputs=[server_status])
	
	return demo

if __name__ == "__main__":
	demo = create_ui()
	demo.launch(
		server_port=7860,
		share=False,
		show_error=True
	)