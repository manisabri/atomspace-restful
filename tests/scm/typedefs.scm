
; DO NOT EDIT THIS FILE!  This file was automatically
; generated from atom definitions in types.script
;
; This file contains basic scheme wrappers for atom creation.
;
(define (Node . x)
	(apply cog-new-node (append (list 'Node) x)))
(define (Link . x)
	(apply cog-new-link (append (list 'Link) x)))
(define (SavingLoadingTestLink . x)
	(apply cog-new-link (append (list 'SavingLoadingTestLink) x)))
(define (SavingLoadingTestNode . x)
	(apply cog-new-node (append (list 'SavingLoadingTestNode) x)))
(define (SentenceNode . x)
	(apply cog-new-node (append (list 'SentenceNode) x)))
(define (DocumentNode . x)
	(apply cog-new-node (append (list 'DocumentNode) x)))
(define (WordNode . x)
	(apply cog-new-node (append (list 'WordNode) x)))
(define (LemmaNode . x)
	(apply cog-new-node (append (list 'LemmaNode) x)))
(define (PartOfSpeechNode . x)
	(apply cog-new-node (append (list 'PartOfSpeechNode) x)))
