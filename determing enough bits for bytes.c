                    bufferbits = querylength % 8; //checking to see if we can fit in full bytes
                                                  //if not we know how many remained bits we need
                                                  //per modbus specification
                    if (bufferbits > 0) {
                    bufferbits = (8 - bufferbits);
                    }
					
					                    //
                    while (bufferbits > 0) {
                        //TODO send_buffer_bit;
                        bufferbits--;
                    }