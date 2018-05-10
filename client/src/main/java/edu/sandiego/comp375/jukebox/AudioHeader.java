package edu.sandiego.comp375.jukebox;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.util.Arrays;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/*
 *Class to construct, send, and hold the AudioHeader between sending
 * the client and the server.
 */
public class AudioHeader {
	//data fields
    private int segType;
    private int dataLength;
    private int dataId;
	/*
	 * Constructing the header using the segment type string and the data ID.
	 * The client will never send data besides the header, so the dataLength
	 * will be 0.
	 *
	 * @param	String segType	A string passed through to set the segType int
	 * @param	int	dataID		An int passed through to set the ID of the
	 * 							song the receiver wants.
	*/
    public AudioHeader(String segType, int dataId) {
		//Setting the segment types from the segType string
        if (segType.equals("PLAY")) {
            this.segType = 1;
        }
        else if (segType.equals("STOP")) {
            this.segType = 2;
        }
        else if (segType.equals("LIST")) {
            this.segType = 3;
        }
        else if (segType.equals("INFO")) {
            this.segType = 4;
        }
        else {
            this.segType = 0;
            System.out.println("ERROR: NOT CORRECT TYPE FOR HEADER");
            System.exit(1);
        }
		//Setting our length to 0 and the ID
        this.dataLength = 0;
        this.dataId = dataId;
    }
	/*
	 * An alternate header to set dataLength 
	 * @param	int segType	an integer of the desired segType
	 * @param	int dataLength	the desired length of the data after the
	 * header
	 * @param	int dataID	the ID the client is inquirying about
	 */
	public AudioHeader(int segType, int dataLength, int dataId){
		this.segType = segType;
		this.dataLength = dataLength;
		this.dataId = dataId;
	}
	/*
	 * A function to create a new AudioHeader from a byte array
	 * @param	byte[] input	the byte[] of the incoming data from the
	 * server
	 */
    public static AudioHeader parseInputHeader(byte[] input) {
       	//Creating a bytebuffer and setting the bit order 
        ByteBuffer buffer = ByteBuffer.wrap(input);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        //getting the desired fields
        int segType = buffer.getInt();
        int dataLength = buffer.getInt();
        int dataId =  buffer.getInt();
		//Debug prints
        System.out.println();
        System.out.println("RECV TYPE: " + segType);
        System.out.println("RECV LENGTH: " + dataLength);
        System.out.println("RECV ID: " + dataId);
        System.out.println();
		//Creating our new AudioHeader and returning it
		AudioHeader newHDR = new AudioHeader(segType, dataLength, dataId);
        return newHDR;
    }

	/*
	 * gets the Header's segment type
	 */
	public int getSegType(){
		return segType;
	}
	/*
	 * gets the Header's data length
	 */
	public int getDataLength(){
		return dataLength;
	}
	/*
	 * gets the Header's data ID
	 */
	public int getDataId(){
		return dataId;
	}
	/*
	 * Sets the Header's segment type
	 * @param int seg	the value used to update the segType
	 */
	public void setSegType(int seg){
		segType = seg;
	}
	/*
	 * Sets the Header's data length
	 * @param int len	the value used to update the dataLength
	 */
	public void setdataLength(int len){
		dataLength = len;
	}
	/*
	 * Sets the Header's dataID
	 * @param int dataID	the value used to update the dataID
	 */
	public void setDataId(int dataId){
		dataId = dataId;
	}
	/*
	 * A function used to send the AudioHeader across a network
	 * @param BufferedOutputStream out 	the output stream that will transmit
	 * the header
	 */
	public void toOutStream(BufferedOutputStream out) {
       //debug prints
        System.out.println("SENT TYPE: " + this.segType);
        System.out.println("SENT LENGTH: " + this.dataLength);
        System.out.println("SENT ID: " + this.dataId);
        System.out.println();
		//setting the segType into a byte array for sending
        byte[] segTypeArr = new byte[4];
		segTypeArr[0] = (byte)(this.segType & 0xFF);
		segTypeArr[1] = (byte)((this.segType >> 8) & 0xFF);
		segTypeArr[2] = (byte)((this.segType >> 16) & 0xFF);
		segTypeArr[3] = (byte)((this.segType >> 24) & 0xFF);
		//setting the dataLength into a byte array for sending
        byte[] dataLengthArr = new byte[4];
		dataLengthArr[0] = (byte)(this.dataLength & 0xFF);
		dataLengthArr[1] = (byte)((this.dataLength >> 8) & 0xFF);
		dataLengthArr[2] = (byte)((this.dataLength >> 16) & 0xFF);
		dataLengthArr[3] = (byte)((this.dataLength >> 24) & 0xFF);
        //setting the dataID into a byte array for sending
        byte[] dataIdArr = new byte[4];
		dataIdArr[0] = (byte)(this.dataId & 0xFF);
		dataIdArr[1] = (byte)((this.dataId >> 8) & 0xFF);
		dataIdArr[2] = (byte)((this.dataId >> 16) & 0xFF);
		dataIdArr[3] = (byte)((this.dataId >> 24) & 0xFF);
		//sending our newly created byte arrays to the server using the output
		//stream
        try {
            out.write(segTypeArr[0]);
			out.write(segTypeArr[1]);
            out.write(segTypeArr[2]);
			out.write(segTypeArr[3]);
		    
            out.write(dataLengthArr[0]);
			out.write(dataLengthArr[1]);
            out.write(dataLengthArr[2]);
			out.write(dataLengthArr[3]);
			
            out.write(dataIdArr[0]);
            out.write(dataIdArr[1]);
			out.write(dataIdArr[2]);
			out.write(dataIdArr[3]);
            
            out.flush();
        }
        catch (Exception e){
            System.out.println("Error writing to out stream");
        }
	}

}
