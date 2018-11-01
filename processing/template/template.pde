// This is an empty Processing sketch with support for Fadecandy.

OPC opc;

void setup()
{
  size(600, 300);
  opc = new OPC(this, "127.0.0.1", 1337);

  // Set up your LED mapping here
  opc.ledGrid8x8(0 * 64, width * 1/8, height * 1/4, height/16, 0, true, false);
}

void draw()
{
  background(0);

  // Draw each frame here
}
