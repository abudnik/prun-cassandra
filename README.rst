Overview
--------
Prun job serialization library.

Building
--------

Build requirements:

- cmake 2.6 (or higher)
- GCC 4.6 (or higher) or Clang
- boost 1.46 (or higher)
- cassandra-cpp-driver-dev

Building debian packages::

> git clone https://github.com/abudnik/prun-cassandra.git
> cd prun-cassandra
> debuild -sa
> ls ../prun-cassandra*.deb

Building RPMs::

> git clone https://github.com/abudnik/prun-cassandra.git
> mv prun-cassandra prun-cassandra-0.1  # add '-version' postfix
> mkdir -p rpmbuild/SOURCES
> tar cjf rpmbuild/SOURCES/prun-cassandra-0.1.tar.bz2 prun-cassandra-0.1
> rpmbuild -ba prun-cassandra-0.1/prun-cassandra-bf.spec
> ls rpmbuild/RPMS/*/*.rpm

Building runtime from sources::

> git clone https://github.com/abudnik/prun-cassandra.git
> cd prun-cassandra
> cmake && make

Cassandra database setup
------------------------

Database can be created using /usr/bin/cqlsh::

> CREATE KEYSPACE IF NOT EXISTS prun WITH replication = {'class': 'SimpleStrategy', 'replication_factor' : 3};
> CREATE TABLE IF NOT EXISTS prun.jobs (job_id text PRIMARY KEY, job_descr text);

Configuration
-------------

Example of prun-cassandra config file: prun/conf/history-cassandra.cfg

Config is represented in the JSON formatted file with following properties:

``remotes`` - name of a single cassandra server node or list of cassandra server nodes delimited by ','
