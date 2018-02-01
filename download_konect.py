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

base_url = 'http://konect.uni-koblenz.de/'
page = requests.get(sys.argv[1])

soup = BeautifulSoup(page.content,"lxml")

title = soup.find('div',{'class': 'title2'}).h1.text
basename = title.lower().replace(" ", "_")

infotext = soup.find('div',{'itemprop': 'description'}).text
infotext = '\n'.join(textwrap.wrap(infotext, width=80))


download_url = soup.find('a', {'title': 'Download TSV file'})['href']
download_url = base_url + '/'.join(download_url.split('/')[1:])


bibtex_url = None
for box in soup.find_all('div', {'class': 'info'}):
    boxtitle = box.find('div', {'class': 'info_title'})
    if not boxtitle or boxtitle.h2.text != "References":
        continue
    for link in box.find_all('a'):
        if link.text == "BibTeX":
            bibtex_url = link['href']
    bibtex_url = base_url + '/'.join(bibtex_url.split('/')[1:])

references = ""
if bibtex_url:
    page = requests.get(bibtex_url)
    references = page.content.decode()

with open('temp/'+basename+'.info', 'w') as filebuf:
    filebuf.write('KONECT: ' + title + '\n')
    filebuf.write(infotext + '\n\n')
    filebuf.write(download_url + '\n\n')
    filebuf.write(references)


archivename = 'temp/'+basename+'.tar.bz2'
networkname = 'temp/'+basename+'.txt'
datafile = download_file(download_url, archivename)
regex = re.compile('.*/out\..*')
with tarfile.open(archivename, 'r') as tfile:
    filenames = tfile.getnames()
    networktar = list(filter(regex.match, filenames))[0]
    tfile.extract(networktar, './temp/')
    shutil.move('temp/'+networktar, networkname)

os.remove(archivename)

test_network(networkname)