The algorithm works as follows:
The initial image is placed into the Background Subtraction image to establish a baseline.  As later frames come through these will be compared with the baseline to identify forground and background.  Additionally, these latter images will be factored in to the adaptive algorithm to be used on the next frame [Background Subtraction].  As a result this preforms best on video with a fixed (non-moving) camera, where one or more forground objects are moving against a static background.  Concurrently, a copy is sent through the Haar clasifier to identify facial regions [Facial Recognition].  The image is then run through a series of quantizing algorithms to produce the "toon" effect [Tooning].  This image is then segmented based on Felzenszwalb's graph cutting algorithm [Segmentation].  Each segment is seperately identified in an array; after this, a single color is chosen for each segment by normalizing the RGB value across the area contained by that segment. Each segment is then transcribed onto its own bitmap for vectorization.  The bitmaps are individually run through the Potrace algorithm to produce a series of vectors.  These vectors are then converted to SVG format and output in XML format to an SVG file [Potrace].

I would like to thank Selinger for taking the time to personally explain the datastructures used to interface with his algorithm.



Background Subtraction:

    @inproceedings{Zivk2004,
        authors="Z.Zivkovic",
        title ="Improved adaptive Gausian mixture model for background subtraction",
        booktitle="Proceedings of the International Conference on Pattern Recognition",
        year=2004,
        abstract="Background subtraction is a common computer vision task. We analyze the usual pixel-level approach. We developan efcient adaptive algorithm using Gaussian mixture probability density. Recursive equations are used to constantly update the parameters and but also to simultaneously select the appropriate number of components for each pixel."
    } 



Facedetection:
Algorithm:

    @inproceedings{citeulike:1281758,
        abstract = {Recently Viola et al. [2001] have introduced a rapid object detection. scheme based on a boosted cascade of simple feature classifiers. In this paper we introduce a novel set of rotated Haar-like features. These novel features significantly enrich the simple features of Viola et al. and can also be calculated efficiently. With these new rotated features our sample face detector shows off on average a 10\% lower false alarm rate at a given hit rate. We also present a novel post optimization procedure for a given boosted cascade improving on average the false alarm rate further by 12.5\%.},
        author = {Lienhart, R.  and Maydt, J. },
        citeulike-article-id = {1281758},
        doi = {10.1109/ICIP.2002.1038171},
        journal = {Image Processing. 2002. Proceedings. 2002 International Conference on},
        keywords = {detection, face, features},
        pages = {I-900--I-903 vol.1},
	      posted-at = {2007-08-08 23:19:44},
	      priority = {2},
	      title = {An extended set of Haar-like features for rapid object detection},
	      url = {http://dx.doi.org/10.1109/ICIP.2002.1038171},
	      volume = {1},
	      year = {2002}
    }

Dataset:

    @techreport{citeulike:1227005,
        abstract = {Recently Viola et al. have introduced a rapid object detection scheme based on a boosted cascade of simple feature classifiers. In this paper we introduce and empirically analysis two extensions to their approach: Firstly, a novel set of rotated haar-like features is introduced. These novel features significantly enrich the simple features of [6] and can also be calculated efficiently. With these new rotated features our sample face detector shows off on average a 10\% lower false alarm rate at a given hit rate. Secondly, we present a through analysis of different boosting algorithms (namely Discrete, Real and Gentle Adaboost) and weak classifiers on the detection performance and computational complexity. We will see that Gentle Adaboost with small CART trees as base classifiers outperform Discrete Adaboost and stumps. The complete object detection training and detection system as well as a trained face detector are available in the Open Computer Vision Library at sourceforge.net [8].},
        author = {Lienhart, Rainer   and Kuranov, Alexander   and Pisarevsky, Vadim  },
        citeulike-article-id = {1227005},
        comment = {from openCV bibliography regarding Haar-cascade},
        institution = {Microprocessor Research Lab, Intel Labs},
        journal = {MRL Technical Report,},
        keywords = {computer\_vision, haar, object\_detection},
        month = {December},
        posted-at = {2007-04-15 02:29:16},
        priority = {4},
        title = {Empirical Analysis of Detection Cascades of Boosted Classifiers for Rapid Object Detection},
        year = {2002}
    }

Tooning:

    @article{1142018,
        author = {Holger Winnem\"{o}ller and Sven C. Olsen and Bruce Gooch},
        title = {Real-time video abstraction},
        journal = {ACM Trans. Graph.},
        volume = {25},
        number = {3},
        year = {2006},
        issn = {0730-0301},
        pages = {1221--1226},
        doi = {http://doi.acm.org/10.1145/1141911.1142018},
        publisher = {ACM},
        address = {New York, NY, USA}
    }

Segmentation:

    @article{981796,
        author = {Pedro F. Felzenszwalb and Daniel P. Huttenlocher},
        title = {Efficient Graph-Based Image Segmentation},
        journal = {Int. J. Comput. Vision},
        volume = {59},
        number = {2},
        year = {2004},
        issn = {0920-5691},
        pages = {167--181},
        doi = {http://dx.doi.org/10.1023/B:VISI.0000022288.19776.77},
        publisher = {Kluwer Academic Publishers},
        address = {Hingham, MA, USA}
    }


Raster to Vector and SVG:	
Selinger, P. 2003. Potrace: a polygon-based tracing algorithm. http://potrace.sourceforge.net/potrace.pdf, September. 

This is not an actual publication, but this is how people seem to be citing it.

