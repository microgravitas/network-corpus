#! /usr/bin/env python3

import sys, re, os
import urllib, shutil
import requests
import textwrap
import tarfile
from bs4 import BeautifulSoup
from test_network import test_network

def download_file(url, filename):
    with urllib.request.urlopen(url) as response, open(filename, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)


request_url = sys.argv[1]

page = None
try:
    page = requests.get(request_url)
except:
    pass

soup = BeautifulSoup(page.content,"lxml")

title = soup.find('th', text='Name').parent.find('td').text
basename = None
if len(sys.argv) >= 3:
    basename = sys.argv[2]
else:
    basename = title.lower().replace(" ", "_")

# The infotext contains some ascii-lines which we remove
infotext = ""
try:
    raw = soup.find('div', {'class': 'notesbox'}).find('pre').text
    infotext = [] 
    for l in raw.split('\n'):
        if len(set(l)) > 1:
            infotext.append(l.strip())
    infotext = '\n'.join(infotext)
except:
    pass

# Also grep subheading for some more information on the network
infotext = soup.find('div', {'class': 'h4'}).text + '\n\n' + infotext

# Can't get the match-by-text on the last anchor, otherwise I'd search
# for 'Matrix Market'. 
download_url = soup.find('th', text='Download').parent.find('td').findAll('a')[2]['href']

with open('temp/'+basename+'.info', 'w') as filebuf:
    filebuf.write('SuiteSparse: ' + title + '\n')
    filebuf.write(infotext + '\n\n')
    filebuf.write(request_url + '\n')
    filebuf.write(download_url + '\n')

archivename = 'temp/'+basename+'.tar.gz'
networkname = 'temp/'+basename+'.txt'
datafile = download_file(download_url, archivename)
regex = re.compile('.*/.*\.mtx')
with tarfile.open(archivename, 'r') as tfile:
    filenames = tfile.getnames()
    print(filenames)
    networktar = list(filter(regex.match, filenames))
    # Select shortest file, since other mtx files have suffixes to describe
    # what additional data they contain
    networktar = sorted(networktar, key=lambda s: len(s))[0]

    tfile.extract(networktar, './temp/')
    shutil.move('temp/'+networktar, networkname)

os.remove(archivename)

# test_network(networkname)