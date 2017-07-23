/*

  RS485_HalfDuplex.pde - example using ModbusMaster library to communicate
  with EPSolar LS2024B controller using a half-duplex RS485 transceiver.

  This example is tested against an EPSolar LS2024B solar charge controller.
  See here for protocol specs:
  http://www.solar-elektro.cz/data/dokumenty/1733_modbus_protocol.pdf

  Library:: ModbusMaster
  Author:: Marius Kintel <marius at kintel dot net>

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/


void read_EnergyMeter()
{
  uint8_t j,a, result;
  uint16_t data[6];
  float x;

  /*
  result = node.readInputRegisters(0x0000, 14);
  if (result == node.ku8MBSuccess)
  {
    Serial.print("V: ");   
    ((byte*)&x)[3]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x00)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x01)& 0xff;
    //Serial.println(x,2);
    Serial.print("I: ");   
    ((byte*)&x)[3]= node.getResponseBuffer(0x06)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x06)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x07)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x07)& 0xff;
    //Serial.println(x,3);
    Serial.print("W: ");   
    ((byte*)&x)[3]= node.getResponseBuffer(0x0C)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x0C)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x0D)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x0D)& 0xff;
    Serial.println(x,2);
  }
  result = node.readInputRegisters(0x001E, 2);
  if (result == node.ku8MBSuccess)
  {
    Serial.print("PF: ");   
    ((byte*)&x)[3]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x00)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x01)& 0xff;
    Serial.println(x,2);
  }
  
  result = node.readInputRegisters(0x0046, 2);
  if (result == node.ku8MBSuccess)
  {
    Serial.print("F: ");   
    ((byte*)&x)[3]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x00)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x01)& 0xff;
    Serial.println(x,2);
  }

  
  result = node.readInputRegisters(0x0156, 2);
  if (result == node.ku8MBSuccess)
  {
    Serial.print("kWh: ");   
    ((byte*)&x)[3]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x00)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x01)& 0xff;
    kwh = x*100; 
    Serial.println(x,2);
  }
  */
}

