const byte BOARD_INIT = 100; //The starting value for board initialization
struct BOARD_PIECE {
  byte num;
  byte nextPiece;
  byte prevPiece;
  Color color;
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

/**
 * Initializes
 */
void initBoard(byte value){
  nextPiece = -1;
  prevPiece = -1;

  //Find the next piece
  FOREACH_FACE(f){
    //Check if this face is connected to another Blink
    if(!isValueReceivedOnFaceExpired(f)){
      nextPiece = f;
      setValueSentOnFace(value + 1);
    }
  }
}
