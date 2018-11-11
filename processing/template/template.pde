// This is an empty Processing sketch with support for Fadecandy.

OPC opc;

void setup()
{
  size(600, 300);
  opc = new OPC(this, "127.0.0.1", 1337, 8);

  // Set up your LED mapping here
  opc.ledStrip(0, 0, 10, width * 1/8, height * 1/9, height/9, 0, false);
  opc.ledStrip(1, 0, 100, width * 1/8, height * 2/9, height/9, 0, false);
  opc.ledStrip(2, 0, 10, width * 1/8, height * 3/9, height/9, 0, false);
  opc.ledStrip(3, 0, 100, width * 1/8, height * 4/9, height/9, 0, false);
  opc.ledStrip(4, 0, 10, width * 1/8, height * 5/9, height/9, 0, false);
  opc.ledStrip(5, 0, 100, width * 1/8, height * 6/9, height/9, 0, false);
  opc.ledStrip(6, 0, 100, width * 1/8, height * 7/9, height/9, 0, false);
  opc.ledStrip(7, 0, 100, width * 1/8, height * 8/9, height/9, 0, false);
  
}

void draw()
{
  background(0);

  // Draw each frame here
}
