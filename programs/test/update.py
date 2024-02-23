import multiprocessing as mp
import requests
import os
import time
import queue
from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
from rich.progress import Progress
from rich.console import Console
import rich
import math

hosts = ['module1', 'module2', 'module3', 'module4', 'controller', 'test2']

class Updater:
    def __init__(self, path, host, proc=mp.Process):
        self._path = path
        self._host = host
        self._bytes_read = 0
        self._file_size = os.path.getsize(path)
        self._running = True
        self._success = False
        self._dt = 0
        self._q = mp.Queue()
        self._proc = proc(target=self._update, args=(self._q, ))
        self._proc.start()

    def host(self):
        return self._host
    
    def upload_time(self):
        self._checkQ()
        return self._dt
    
    def progress(self):
        self._checkQ()
        progress = self._bytes_read / self._file_size
        if progress > 1:
            progress = 1
        return progress
    
    def running(self):
        self._checkQ()
        return self._running
    
    def successful(self):
        self._checkQ()
        return self._success
    
    def _checkQ(self):
        try:
            while self._q.qsize() > 0:
                data = self._q.get_nowait()
                if 'bytes_read' in data.keys():
                    self._bytes_read = data['bytes_read']
                elif 'success' in data.keys():
                    self._success = data['success']
                elif 'dt' in data.keys():
                    self._dt = data['dt']
                elif 'running' in data.keys():
                    self._running = data['running']
                    if not self._running:
                        self._proc.join()
        except queue.Empty:
            pass
        
    def _updateCB(self, monitor):
        self._q.put({'bytes_read': monitor.bytes_read})
    
    def _update(self, q):
        start = time.time()
        self._q = q
        try:
            encoder = MultipartEncoder(fields={'file': (os.path.basename(self._path), open(self._path, 'rb'))})
            monitor = MultipartEncoderMonitor(encoder, self._updateCB)
            url = f'http://{self._host}.local/ota/update'
            response = requests.post(url, data=monitor, headers={'Content-Type': encoder.content_type})
            if "update successful" in response.text:
                self._q.put({'success': True})
        except:
            pass
        self._q.put({'dt': time.time() - start})
        self._q.put({'running': False})

def mass_module_update(path, hosts, processes=1):
    batches = math.ceil(len(hosts) / processes)
    processed = 0
    console = Console()

    start = time.time()
    for batch in range(batches):
        console.log(f'[green]Starting [white]Batch [yellow]{batch + 1} [white]/ [yellow]{batches} [white]...')

        num_hosts = min(len(hosts) - processed, processes)
        updaters = [Updater(path, host) for host in hosts[processed:processed + num_hosts]]
        processed += num_hosts
        done = [False for _ in updaters]
        updating = True
        
        with Progress() as progress:
            bar = progress.add_task("[green]Uploading...", total=1)

            while updating:
                upload_progress = 0
                updating = False

                for (i, updater) in enumerate(updaters):
                    if not updater.running():
                        upload_progress += 1
                    else:
                        upload_progress += updater.progress()
                    
                    if not updater.running():
                        if not done[i]:
                            if updater.successful():
                                progress.log(f'[green]upload >> [white]{updater.host()} in [yellow]{"{:.2f}".format(updater.upload_time())} [white]seconds')
                            else:
                                progress.log(f'[red]upload >> [white]{updater.host()} in [yellow]{"{:.2f}".format(updater.upload_time())} [white]seconds')
                            done[i] = True
                    else:
                        updating = True
                
                upload_progress = upload_progress / len(updaters)
                progress.update(bar, completed=upload_progress)
            
            progress.update(bar, description=f'[green]Done Uploading', completed=1)
    
    console.log(f'[green]Finished [white]uploading to [yellow]{len(hosts)} [white]host{"" if len(hosts) == 1 else "s"} in [yellow]{"{:.2f}".format(time.time() - start)} [white]seconds')


header = """[blue1]
  ____         __ _                 _            
 / ___|  ___  / _| |___      ____ _| |_ ___ _ __ 
 \___ \ / _ \| |_| __\ \ /\ / / _` | __/ _ \ '__|
  ___) | (_) |  _| |_ \ V  V / (_| | ||  __/ |   
 |____/ \___/|_|  \__| \_/\_/ \__,_|\__\___|_|   [orange_red1]
  _____ _                                        
 |  ___(_)_ __ _ __ _____      ____ _ _ __ ___   
 | |_  | | '__| '_ ` _ \ \ /\ / / _` | '__/ _ \  
 |  _| | | |  | | | | | \ V  V / (_| | | |  __/  
 |_|   |_|_|  |_| |_| |_|\_/\_/ \__,_|_|  \___|  [blue1]
  _   _       _                 _                
 | | | |_ __ | | ___   __ _  __| | ___ _ __      
 | | | | '_ \| |/ _ \ / _` |/ _` |/ _ \ '__|     
 | |_| | |_) | | (_) | (_| | (_| |  __/ |        
  \___/| .__/|_|\___/ \__,_|\__,_|\___|_|        
       |_|                                       
"""


if __name__ == '__main__':
    rich.print(header)

    f = None
    dir = './build/esp32.esp32.esp32/'
    for file in os.listdir(dir):
        if '.ino.bin' in file:
            f = os.path.abspath(f'{dir}/{file}')
            break
    
    if f is None:
        rich.print("No BIN file found")
        exit()
    else:
        rich.print(f"Uploading {f}")

    mass_module_update(f, hosts, len(hosts))
