/*
 * Simple Open Pixel Control client for Processing,
 * designed to sample each LED's color from some point on the canvas.
 *
 * Micah Elizabeth Scott, 2013
 * This file is released into the public domain.
 *
 * Modified by Shlomo Zippel to support tenere controller
 * using OPC over UDP
 */

import java.net.*;
import java.util.Arrays;
import java.util.List;
import java.util.ArrayList;

public class OPC implements Runnable
{
  Thread thread;
  DatagramSocket socket;
  OutputStream output, pending;
  String host;
  int port;
  InetAddress addr;

  int numStrips;
  List<int[]> pixelLocations;
  byte[] packetData;
  byte firmwareConfig;
  String colorCorrection;
  boolean enableShowLocations;

  OPC(PApplet parent, String host, int port, int numStrips)
  {
    this.host = host;
    this.port = port;
    try {
      socket = new DatagramSocket();
      addr = InetAddress.getByName(host);
    } catch(Exception e) {
    }
    
    this.numStrips = numStrips;
    this.pixelLocations = new ArrayList<int[]>(numStrips);
    for (int s=0; s<numStrips; s++) {
      this.pixelLocations.add(s, new int[0]);
    }
    
    // tenere controller uses an ethernet controller with a fixed MTU and simply
    // drops IP fragments. We will try and get as many strips as we can into
    // each UDP packet
    packetData = new byte[1472];
    
    thread = new Thread(this);
    thread.start();
    this.enableShowLocations = true;
    parent.registerMethod("draw", this);
  }

  // Set the location of a single LED
  void led(int strip, int index, int x, int y)  
  {
    // For convenience, automatically grow the pixelLocations array. We do want this to be an array,
    // instead of a HashMap, to keep draw() as fast as it can be.
    if (index >= pixelLocations.get(strip).length) {
      pixelLocations.set(strip, Arrays.copyOf(pixelLocations.get(strip), index + 1));
    }

    pixelLocations.get(strip)[index] = x + width * y;
  }
  
  // Set the location of several LEDs arranged in a strip.
  // Angle is in radians, measured clockwise from +X.
  // (x,y) is the center of the strip.
  void ledStrip(int strip, int index, int count, float x, float y, float spacing, float angle, boolean reversed)
  {      
    float s = sin(angle);
    float c = cos(angle);
    for (int i = 0; i < count; i++) {
      led(strip, reversed ? (index + count - 1 - i) : (index + i),
        (int)(x + (i - (count-1)/2.0) * spacing * c + 0.5),
        (int)(y + (i - (count-1)/2.0) * spacing * s + 0.5));
    }
  }

  // Set the locations of a ring of LEDs. The center of the ring is at (x, y),
  // with "radius" pixels between the center and each LED. The first LED is at
  // the indicated angle, in radians, measured clockwise from +X.
  void ledRing(int strip, int index, int count, float x, float y, float radius, float angle)
  {
    for (int i = 0; i < count; i++) {
      float a = angle + i * 2 * PI / count;
      led(strip, index + i, (int)(x - radius * cos(a) + 0.5),
        (int)(y - radius * sin(a) + 0.5));
    }
  }

  // Should the pixel sampling locations be visible? This helps with debugging.
  // Showing locations is enabled by default. You might need to disable it if our drawing
  // is interfering with your processing sketch, or if you'd simply like the screen to be
  // less cluttered.
  void showLocations(boolean enabled)
  {
    enableShowLocations = enabled;
  }
  

  // Automatically called at the end of each draw().
  // This handles the automatic Pixel to LED mapping.
  // If you aren't using that mapping, this function has no effect.
  // In that case, you can call setPixelCount(), setPixel(), and writePixels()
  // separately.
  void draw()
  {
    loadPixels();
    
    int currentOffset = 0;
    
    for (int s=0; s<this.numStrips; s++) {
      int numPixels = pixelLocations.get(s).length;
      int numBytes = numPixels * 3;

      // skip blank strips
      if  (numPixels == 0) {
        continue;
      }
      
      // is this packet full? send it off!
      if (currentOffset + numBytes + 4 > packetData.length) {
        writePixels(currentOffset);
        currentOffset = 0;
      }
      
      // write header for this strip
      packetData[currentOffset+0] = (byte)s;                 // Channel
      packetData[currentOffset+1] = (byte)0x00;              // Command (Set pixel colors)
      packetData[currentOffset+2] = (byte)(numBytes >> 8);   // Length high byte
      packetData[currentOffset+3] = (byte)(numBytes & 0xFF); // Length low byte

      currentOffset += 4;
      int[] thisStrip = pixelLocations.get(s);
      for (int i = 0; i < numPixels; i++) {
        int pixelLocation = thisStrip[i];
        int pixel = pixels[pixelLocation];

        packetData[currentOffset + 0] = (byte)(pixel >> 16);
        packetData[currentOffset + 1] = (byte)(pixel >> 8);
        packetData[currentOffset + 2] = (byte)pixel;
        currentOffset += 3;
      }
    }
        
    writePixels(currentOffset);
        
    if (enableShowLocations) {
      updatePixels();
    }
  }
  
  // Transmit our current buffer of pixel values to the OPC server. This is handled
  // automatically in draw() if any pixels are mapped to the screen, but if you haven't
  // mapped any pixels to the screen you'll want to call this directly.
  void writePixels(int len)
  {
    if (socket == null || len == 0) {
      return;
    }

    try {
      DatagramPacket packet = new DatagramPacket(packetData, len, this.addr, this.port);
      socket.send(packet);
    } catch (Exception e) {
      
    }
  }

  public void run()
  {
  }
}
