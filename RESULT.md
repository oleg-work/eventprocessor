```
Producer 0 ──► [Ring 0] ──┐
Producer 1 ──► [Ring 1] ──┼──► Consumer (polls all rings)
Producer 2 ──► [Ring 2] ──┤
...
Producer N ──► [Ring N] ──┘
```

Chip: Apple M3 Max  
Total Number of Cores: 16 (12 Performance and 4 Efficiency)

```bash
Consumer:
        ProcessedEvents: 160000000
        DuringFailedPolls: 1
        DuringSuccessPolls: 625294
        AfterFailedPolls: 0
        AfterSuccessPolls: 0
        SimpleEvent:
        c 0 | d 156854680 | p 156854680
        ComplexEvent:
        c 0 | d 1714595 | p 1714595
        StatsEvent:
        c 0 | d 1430725 | p 1430725
        TotalEvents:
        c 0 | d 160000000 | p 160000000
Producer: 0
        ProcessedEvents: 10000000
        DuringFailedPolls: 6813429
        DuringSuccessPolls: 417682
        SimpleEvent:
        c 9801920 | d 0 | p 0
        ComplexEvent:
        c 107759 | d 0 | p 0
        StatsEvent:
        c 90321 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 1
        ProcessedEvents: 10000000
        DuringFailedPolls: 4044230
        DuringSuccessPolls: 415797
        SimpleEvent:
        c 9802299 | d 0 | p 0
        ComplexEvent:
        c 107842 | d 0 | p 0
        StatsEvent:
        c 89859 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 2
        ProcessedEvents: 10000000
        DuringFailedPolls: 4583402
        DuringSuccessPolls: 416109
        SimpleEvent:
        c 9802271 | d 0 | p 0
        ComplexEvent:
        c 108154 | d 0 | p 0
        StatsEvent:
        c 89575 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 3
        ProcessedEvents: 10000000
        DuringFailedPolls: 4451362
        DuringSuccessPolls: 416481
        SimpleEvent:
        c 9802085 | d 0 | p 0
        ComplexEvent:
        c 108036 | d 0 | p 0
        StatsEvent:
        c 89879 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 4
        ProcessedEvents: 10000000
        DuringFailedPolls: 3220058
        DuringSuccessPolls: 415437
        SimpleEvent:
        c 9802169 | d 0 | p 0
        ComplexEvent:
        c 108262 | d 0 | p 0
        StatsEvent:
        c 89569 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 5
        ProcessedEvents: 10000000
        DuringFailedPolls: 3059134
        DuringSuccessPolls: 416049
        SimpleEvent:
        c 9801750 | d 0 | p 0
        ComplexEvent:
        c 108366 | d 0 | p 0
        StatsEvent:
        c 89884 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 6
        ProcessedEvents: 10000000
        DuringFailedPolls: 2646386
        DuringSuccessPolls: 414493
        SimpleEvent:
        c 9802603 | d 0 | p 0
        ComplexEvent:
        c 107897 | d 0 | p 0
        StatsEvent:
        c 89500 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 7
        ProcessedEvents: 10000000
        DuringFailedPolls: 3058859
        DuringSuccessPolls: 416000
        SimpleEvent:
        c 9801583 | d 0 | p 0
        ComplexEvent:
        c 108585 | d 0 | p 0
        StatsEvent:
        c 89832 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 8
        ProcessedEvents: 10000000
        DuringFailedPolls: 3091265
        DuringSuccessPolls: 415762
        SimpleEvent:
        c 9801783 | d 0 | p 0
        ComplexEvent:
        c 108488 | d 0 | p 0
        StatsEvent:
        c 89729 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 9
        ProcessedEvents: 10000000
        DuringFailedPolls: 1773632
        DuringSuccessPolls: 414423
        SimpleEvent:
        c 9802469 | d 0 | p 0
        ComplexEvent:
        c 108272 | d 0 | p 0
        StatsEvent:
        c 89259 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 10
        ProcessedEvents: 10000000
        DuringFailedPolls: 2323877
        DuringSuccessPolls: 414062
        SimpleEvent:
        c 9802602 | d 0 | p 0
        ComplexEvent:
        c 108184 | d 0 | p 0
        StatsEvent:
        c 89214 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 11
        ProcessedEvents: 10000000
        DuringFailedPolls: 2832889
        DuringSuccessPolls: 414410
        SimpleEvent:
        c 9802663 | d 0 | p 0
        ComplexEvent:
        c 108097 | d 0 | p 0
        StatsEvent:
        c 89240 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 12
        ProcessedEvents: 10000000
        DuringFailedPolls: 1967711
        DuringSuccessPolls: 415212
        SimpleEvent:
        c 9801839 | d 0 | p 0
        ComplexEvent:
        c 108537 | d 0 | p 0
        StatsEvent:
        c 89624 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 13
        ProcessedEvents: 10000000
        DuringFailedPolls: 2477667
        DuringSuccessPolls: 414640
        SimpleEvent:
        c 9802408 | d 0 | p 0
        ComplexEvent:
        c 108248 | d 0 | p 0
        StatsEvent:
        c 89344 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 14
        ProcessedEvents: 10000000
        DuringFailedPolls: 2342970
        DuringSuccessPolls: 414141
        SimpleEvent:
        c 9802589 | d 0 | p 0
        ComplexEvent:
        c 108083 | d 0 | p 0
        StatsEvent:
        c 89328 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

Producer: 15
        ProcessedEvents: 10000000
        DuringFailedPolls: 56008953
        DuringSuccessPolls: 384194
        SimpleEvent:
        c 9821647 | d 0 | p 0
        ComplexEvent:
        c 91785 | d 0 | p 0
        StatsEvent:
        c 86568 | d 0 | p 0
        TotalEvents:
        c 10000000 | d 0 | p 0

General:
        TotalProcessedEvents/kExpectedProcessedEvents: 160000000/160000000
        Consumer 1:     255 events/poll;        fail rate: 0.000159925%
        Producer 0:     23 events/poll;         fail rate: 94.2238%
        Producer 1:     24 events/poll;         fail rate: 90.6773%
        Producer 2:     24 events/poll;         fail rate: 91.677%
        Producer 3:     24 events/poll;         fail rate: 91.4442%
        Producer 4:     24 events/poll;         fail rate: 88.5728%
        Producer 5:     24 events/poll;         fail rate: 88.028%
        Producer 6:     24 events/poll;         fail rate: 86.4584%
        Producer 7:     24 events/poll;         fail rate: 88.0283%
        Producer 8:     24 events/poll;         fail rate: 88.1449%
        Producer 9:     24 events/poll;         fail rate: 81.0598%
        Producer 10:    24 events/poll;         fail rate: 84.8769%
        Producer 11:    24 events/poll;         fail rate: 87.2383%
        Producer 12:    24 events/poll;         fail rate: 82.5755%
        Producer 13:    24 events/poll;         fail rate: 85.664%
        Producer 14:    24 events/poll;         fail rate: 84.9792%
        Producer 15:    26 events/poll;         fail rate: 99.3187%
        Elapsed time: 900 ms
        Throughput: 177.668M events/s # https://github.com/boonzy00/ringmpsc/blob/main/docs/benchmarks.md
                                      # Performance is within the expected order of magnitude,
                                      # even without additional optimizations.
        PerEvent: 5.62849 ns/event
        StatsEvent processing durations:
        Count: 1430725
        P50: 67791 ns | 67.791 us
        P90: 141750 ns | 141.75 us
        P95: 144125 ns | 144.125 us
        P99: 160625 ns | 160.625 us
        Avg: 82304 ns | 82.304 us
```
