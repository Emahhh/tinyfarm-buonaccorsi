#! /usr/bin/env python3
# gestisce più clienti contemporaneamente usando i thread
import sys, struct, socket, threading, os

# host e porta di default
HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 59885  # Port to listen on (non-privileged ports are > 1023)
debugPrint = False


# codice da eseguire nei singoli thread 
class ClientThread(threading.Thread):

    def __init__(self, conn, addr):
        threading.Thread.__init__(self)
        self.conn = conn
        self.addr = addr

    def run(self):
      if debugPrint:
        print("====", self.ident, "mi occupo di", self.addr)

      gestisci_connessione(self.conn, self.addr)

      if debugPrint:
        print("====", self.ident, "ho finito")




def main(host=HOST,port=PORT):
  # creiamo il server socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:
      s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      s.bind((host, port))
      s.listen()
      print("In attesa del primo client...")

      while True:
        conn, addr = s.accept()
        t = ClientThread(conn,addr)
        t.start()
 
    except KeyboardInterrupt: # se ricevo un messaggio di terminazione speciale da MasterWorker
        print("\nTerminazione con KeyboardInterrupt!")
        pass
    except Exception as e:
        print("Terminazione con Exception generica!")
        print(e)
        pass
    finally:
        print('Va bene smetto...')
        s.shutdown(socket.SHUT_RDWR)

    

  
# gestisci una singola connessione con un client
def gestisci_connessione(conn,addr): 
  # in questo caso potrei usare direttamente conn e l'uso di with serve solo a garantire che conn venga chiusa all'uscita del blocco ma in generale with esegue le necessarie inzializzazione e il clean-up finale
  with conn:
    if debugPrint:  
      print(f"Contattato da {addr}")

    # attendo lungh: un 64bit unsigned long - la lunghezza della prossima stringa
    daRicevere = 8
    data = recv_all(conn, daRicevere)
    assert len(data)==daRicevere
    lungh = struct.unpack("!Q", data)[0]

    # attendo una stringa lunga lungh byte
    daRicevere = lungh
    data = recv_all(conn, daRicevere)
    assert len(data)==daRicevere
    filename = data.decode('ASCII')
    
    # ---- attendo somma: un long da 64 bit = 8 byte
    daRicevere = 8
    data = recv_all(conn, daRicevere)
    assert len(data)==daRicevere
    somma  = struct.unpack("!q", data)[0]
    
    if somma == 0 and filename == "TerminaServer!!!?":
       print("Terminazione per richiesta di terminazione remota!")
       os._exit(0) # termina anche il processo principale
       return # mai raggiunto

    print(somma, filename)
 
 
 

# riceve esattamente n byte e li restituisce in un array di byte - il tipo restituto è "bytes": una sequenza immutabile di valori 0-255
def recv_all(conn, n):
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