# Producer/Consumer: Discman's Pattern {#Producer/Consumer}

In Producer/Consumer, one object generates data (the Producer), and one or more objects
consume it (the Consumers). When a consumer calls consume(), the next datum is
retrieved for it from the producer.

## Producers

The only producer in Discman is CDDrive. It produces int16_t audio samples.

## Consumers

There are two consumers of CDDrive:

- AudioOutput
  Retrieves audio samples from CDDrive and passes them to the audio device

- CDRipper
  Retrieves audio samples from CDDrive and writes them to one or more files (one file per track)

## One or Many Consumers

A Producer can have multiple Consumers if the underlying data store can support it. In this case, the
Producer provides a method to register the Consumer back with the Producer. The Producer must be able to serve data at different offsets in the underlying data store any time it is requested by a given Consumer.

When a Producer has multiple Consumers, each Consumer must use the overloaded method Producer::next(const Consumer* const). When a Producer has just one Consumer, either version of Producer::next() can be called.

CDDrive produces audio samples from a CD-ROM using buffered sequential access. As new data is read in sequentially, old data is discarded. While theoretically possible, supporting multiple Consumers in CDDrive would cause <a href="https://www.datacore.com/glossary/disk-thrashing/" target="_blank">disc thrashing</a>. The application performance degrades substantially since the disc drive is constantly being asked to seek.