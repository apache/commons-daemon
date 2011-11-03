/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ====================================================================
 * Contributed by Mladen Turk <mturk@apache.org>
 * 05 Aug 2003
 * ====================================================================
 */

#ifndef _PRUNMGR_H
#define _PRUNMGR_H

#undef  PRG_VERSION
#define PRG_VERSION    "1.0.8.0"
#define PRG_REGROOT   L"Apache Software Foundation\\Procrun 2.0"

#define IDM_TM_EXIT                     2000
#define IDM_TM_START                    2001
#define IDM_TM_STOP                     2002
#define IDM_TM_PAUSE                    2003
#define IDM_TM_RESTART                  2004
#define IDM_TM_CONFIG                   2005
#define IDM_TM_ABOUT                    2006
#define IDM_TM_DUMP                     2007

#define IDMS_REFRESH                    2020

#define IDI_ICONSTOP                    2030
#define IDI_ICONRUN                     2031



/* Property pages */

#define IDD_PROPPAGE_SGENERAL           2600
#define IDC_PPSGNAME                    2601
#define IDC_PPSGDISP                    2602
#define IDC_PPSGDESC                    2603
#define IDC_PPSGDEXE                    2604
#define IDC_PPSGCMBST                   2605
#define IDC_PPSGSTATUS                  2606
#define IDC_PPSGSTART                   2607
#define IDC_PPSGSTOP                    2608
#define IDC_PPSGPAUSE                   2609
#define IDC_PPSGRESTART                 2610

#define IDD_PROPPAGE_LOGON              2620
#define IDC_PPSLLS                      2621
#define IDC_PPSLID                      2622
#define IDC_PPSLUA                      2623
#define IDC_PPSLUSER                    2624
#define IDC_PPSLBROWSE                  2625
#define IDC_PPSLPASS                    2626
#define IDC_PPSLCPASS                   2627
#define IDL_PPSLPASS                    2628
#define IDL_PPSLCPASS                   2629

#define IDD_PROPPAGE_LOGGING            2640
#define IDC_PPLGLEVEL                   2641
#define IDC_PPLGPATH                    2642
#define IDC_PPLGBPATH                   2643
#define IDC_PPLGPREFIX                  2644
#define IDC_PPLGPIDFILE                 2645
#define IDC_PPLGSTDOUT                  2646
#define IDC_PPLGBSTDOUT                 2647
#define IDC_PPLGSTDERR                  2648
#define IDC_PPLGBSTDERR                 2649

#define IDD_PROPPAGE_JVM                2660
#define IDC_PPJAUTO                     2661
#define IDC_PPJJVM                      2662
#define IDC_PPJBJVM                     2663
#define IDC_PPJCLASSPATH                2664
#define IDC_PPJOPTIONS                  2665
#define IDC_PPJMS                       2666
#define IDC_PPJMX                       2667
#define IDC_PPJSS                       2668

#define IDD_PROPPAGE_START              2680
#define IDC_PPRCLASS                    2681
#define IDC_PPRIMAGE                    2682
#define IDC_PPRBIMAGE                   2683
#define IDC_PPRWPATH                    2684
#define IDC_PPRBWPATH                   2685
#define IDC_PPRMETHOD                   2686
#define IDC_PPRARGS                     2687
#define IDC_PPRTIMEOUT                  2688
#define IDC_PPRMODE                     2689

#define IDD_PROPPAGE_STOP               2700
#define IDC_PPSCLASS                    2701
#define IDC_PPSIMAGE                    2702
#define IDC_PPSBIMAGE                   2703
#define IDC_PPSWPATH                    2704
#define IDC_PPSBWPATH                   2705
#define IDC_PPSMETHOD                   2706
#define IDC_PPSARGS                     2707
#define IDC_PPSTIMEOUT                  2708
#define IDC_PPSMODE                     2709

#define IDS_ALREAY_RUNING               3100
#define IDS_ERRORCMD                    3101
#define IDS_HSSTART                     3102
#define IDS_HSSTOP                      3103
#define IDS_HSPAUSE                     3104
#define IDS_HSRESTART                   3105
#define IDS_VALIDPASS                   3106
#define IDS_PPGENERAL                   3107
#define IDS_PPLOGON                     3108
#define IDS_PPLOGGING                   3109
#define IDS_PPJAVAVM                    3110
#define IDS_PPSTART                     3111
#define IDS_PPSTOP                      3112
#define IDS_LGPATHTITLE                 3113
#define IDS_ALLFILES                    3114
#define IDS_DLLFILES                    3115
#define IDS_EXEFILES                    3116
#define IDS_LGSTDERR                    3117
#define IDS_LGSTDOUT                    3118
#define IDS_PPJBJVM                     3119
#define IDS_PPWPATH                     3120
#define IDS_PPIMAGE                     3121
#define IDS_ERRSREG                     3122

#define IDS_NOTIMPLEMENTED              3199

#endif /* _PRUNMGR_H */

