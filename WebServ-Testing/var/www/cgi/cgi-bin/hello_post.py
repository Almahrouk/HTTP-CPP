import sys
import urllib.parse


body = sys.stdin.read() or ""
params = urllib.parse.parse_qs(body, keep_blank_values=True)

name = params.get("name", [""])[0]
email = params.get("email", [""])[0]

print("Content-type: text/html\r\n\r\n")


print(f"<p>HELLO {name} with email {email} from CGI</p>")
print("<br/>")
# while(1)
#     continue
print(f"<p>BYE {name} with email {email} from CGI</p>")
# Command to test
#  curl -v -X POST \
#   -H "Content-Type: application/x-www-form-urlencoded" \
#   -d "name=alice&email=rabat@example.com" \
#   http://localhost:4242/cgi/hello_post.py

