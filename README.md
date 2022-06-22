Code written by Jaden Breeland and Ian McCleary
This project is a multi-party chat room. The project is split up into the server who manages the chatroom and all messages and the client is split into
a contoller and view called particpant and observer. 
server usage: ./server <port to listen for particpants> <port to listen for observers>
particpant/observer usage: ./<particpant|observer> <server ip address> <server port>
particpants need to create a unique username less that 11 characters to enter and observers need to bound to an existing participant username to enter.
