#! /usr/bin/env python3
# gestisce più clienti contemporaneamente usando i thread
import sys, struct, socket, threading

# host e porta di default
HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 59885  # Port to listen on (non-privileged ports are > 1023)
 

# codice da eseguire nei singoli thread 
class ClientThread(threading.Thread):

    def __init__(self, conn, addr):
        threading.Thread.__init__(self)
        self.conn = conn
        self.addr = addr

    def run(self):
      print("====", self.ident, "mi occupo di", self.addr)
      gestisci_connessione(self.conn, self.addr)
      print("====", self.ident, "ho finito")



def main(host=HOST,port=PORT):
  # creiamo il server socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:  
      s.bind((host, port))
      s.listen()

      while True:
        print("In attesa di un client...")
        conn, addr = s.accept()
        t = ClientThread(conn,addr)
        t.start()
 
    except KeyboardInterrupt: # se ricevo un messaggio di terminazione speciale da MasterWorker
        print("\nTerminazione con KeyboardInterrupt!")
        pass
    except Exception as e:
        print("\nTerminazione CON Exception generica!")
        print(e)
        pass
    finally:
        print('Va bene smetto...')
        s.shutdown(socket.SHUT_RDWR)

    
    

# gestisci una singola connessione con un client
def gestisci_connessione(conn,addr): 
  # in questo caso potrei usare direttamente conn e l'uso di with serve solo a garantire che conn venga chiusa all'uscita del blocco ma in generale with esegue le necessarie inzializzazione e il clean-up finale
  with conn:  
    print(f"Contattato da {addr}")
    # ---- attendo un long da 64 bit = 8 byte e una stringa da 256 byte
    daRicevere = 8+256
    data = recv_all(conn, daRicevere)
    assert len(data)==daRicevere
    somma  = struct.unpack("!q",data[:8])[0]
    nomefile = struct.unpack("!s",data[8:])[0]
    print(f"Ho ricevuto i valori:", inizio, nomefile)
 
 


# riceve esattamente n byte e li restituisce in un array di byte
# il tipo restituto è "bytes": una sequenza immutabile di valori 0-255
# analoga alla readn che abbiamo visto nel C
def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
        raise RuntimeError("socket connection broken")

    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks
 



if len(sys.argv)==1:
  main()
else:
  print("Il server NON prende argomenti sulla linea di comando. Uso:\n\t %s" % sys.argv[0])