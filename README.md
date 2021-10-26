# I * LOVE * PIE

Author: George Whitfield

Design: You LOVE pie. In fact, you love it so much that you are willing to eat over
200 pies in the span of a few minutes. However, your friends LOVE pie too. Whoever eats the pies first will be crowned the supreme pie overlord. 

Networking: 
-> clients send server data about number of pies collected and the client's position in the scene
-> the server sends a message to all the clients displaying the progress of the pie 
collection
-> the server sends a list of all of the enemy positions to the client so that the client
   can see where their enemies are in the game
-> if a client has collected all the pies, then the client will send a 1 byte flag to the server and the server will update accordingly


Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

Use WASD to move. Mouse movement to look around. Eat the pies.

Sources:

This game was built with [NEST](NEST.md).

