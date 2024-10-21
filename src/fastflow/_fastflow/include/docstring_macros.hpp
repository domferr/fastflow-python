#define doc(docname, signature, description) \
    PyDoc_STRVAR(   \
        docname,  \
        signature "\n" \
        "--\n"  \
        "\n"    \
        description \
    )

#define ffTime_doc(docname, bbname) doc(\
    docname, \
    "ffTime(self, /)", \
    "Return the execution time (in milliseconds) spent by the " # bbname   \
)

#define run_doc(docname, bbname) doc(\
    docname, \
    "run(self, /)", \
    "Run the " # bbname " asynchronously"   \
)

#define run_and_wait_end_doc(docname, bbname) doc(\
    docname, \
    "run_and_wait_end(self, /)", \
    "Run and wait the end of the " # bbname \
)

#define wait_doc(docname, bbname) doc(\
    docname, \
    "wait(self, /)", \
    "Wait for the " # bbname " to complete all its tasks" \
)

#define submit_doc(docname, bbname) doc(\
    docname, \
    "submit(self, object, /)", \
    "Submit one object to the first stage of the " # bbname \
)

#define collect_next_doc(docname, bbname) doc(\
    docname, \
    "collect_next(self, /)", \
    "Returns\n" \
    "-------\n" \
    "data : any type\n" \
    "    Data collected.\n"    \
    "\n"    \
    "Collect next output data of the " # bbname \
)

#define blocking_mode_doc(docname, bbname) doc(\
    docname, \
    "blocking_mode(self, boolean, /)", \
    "Enable or disable the blocking mode of the " # bbname \
)

#define no_mapping_doc(docname, bbname) doc(\
    docname, \
    "no_mapping(self, /)", \
    "Disable mapping mode of the " # bbname \
)