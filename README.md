# taxdb
Get blast names from NCBI taxdb.

## Build

```
gcc -o taxdb taxdb.c
```

## Prepare taxdb files

```
wget ftp://ftp.ncbi.nlm.nih.gov/blast/db/taxdb.tar.gz
tar -xf taxdb.tar.gz
```

## Run
```
$ ./taxdb -i taxdb.bti -d taxdb.btd -t 1280
#TaxID	ScientificName	CommonName	BlastName	Kingdom
1280	Staphylococcus aureus	Staphylococcus aureus	firmicutes	Bacteria

$ ./taxdb -i taxdb.bti -d taxdb.btd -t 1280,562
#TaxID	ScientificName	CommonName	BlastName	Kingdom
1280	Staphylococcus aureus	Staphylococcus aureus	firmicutes	Bacteria
562	Escherichia coli	Escherichia coli	enterobacteria	Bacteria

$ cat tids.txt
1280
562
6666

$ ./taxdb -i taxdb.bti -d taxdb.btd -t tids.txt
#TaxID	ScientificName	CommonName	BlastName	Kingdom
1280	Staphylococcus aureus	Staphylococcus aureus	firmicutes	Bacteria
562	Escherichia coli	Escherichia coli	enterobacteria	Bacteria
6666	n/a	n/a	n/a	n/a

$ ./taxdb -i /opt/dat/taxdb.bti -d /opt/dat/taxdb.btd -t tids.txt -t 666
#TaxID	ScientificName	CommonName	BlastName	Kingdom
1280	Staphylococcus aureus	Staphylococcus aureus	firmicutes	Bacteria
562	Escherichia coli	Escherichia coli	enterobacteria	Bacteria
6666	n/a	n/a	n/a	n/a
666	Vibrio cholerae	Vibrio cholerae	g-proteobacteria	Bacteria
```