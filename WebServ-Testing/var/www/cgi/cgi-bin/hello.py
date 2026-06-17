import sys
import os


server_port = os.getenv("SERVER_PORT")
server_name = os.getenv("SERVER_NAME")
query_string = os.getenv("QUERY_STRING")
cookie = os.getenv("COOKIE")


print("Content-type: text/html\r\n\r\n")

print("<html><body>")

print("<h1>HELLO from CGI</h1>")
print("<h2>Starting CGI process...</h2>")

print("<br>")
sys.stdout.flush()

print(f"<p>query_string : {query_string}</p>")
print(f"<p>cookie : {cookie}</p>")
print(f"<p>server_port : {server_port}</p>")
print(f"<p>server_name : {server_name}</p>")
sys.stdout.flush()

print("<br>")
sys.stdout.flush()


# while(1):
#     continue
# time.sleep(4)  # Sleep before final write

print("<h1>By from CGI</h1>")
print("</body></html>")
sys.stdout.flush()
