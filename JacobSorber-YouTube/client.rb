require 'socket'

s = TCPSocket.new 'localhost', 18001

# s.write("/tmp/testfiles/#{ARGV[0]}.c\n")
s.write("/tmp/#{ARGV[0]}\n")

s.each_line do |line|
    puts line
end

s.close