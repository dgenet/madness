#include <madness/madness_config.h>

#ifdef HAVE_PARSEC
#include "parsec_madness.h"
#include <iostream>
#include "thread.h"
#include <parsec/parsec_internal.h>

extern "C" int fake_update_runtime_task( parsec_taskpool_t *parsec_taskpool, int32_t nb_tasks );

int fake_update_runtime_task( parsec_taskpool_t *parsec_taskpool, int32_t nb_tasks ) { (void)parsec_taskpool; (void)nb_tasks; return 0; }

// Here we initialize with the right child class
namespace madness {
    parsec_hook_return_t complete_madness_task_execution (parsec_execution_stream_t *es, 
                                                         parsec_task_t *task)
    {
        (void)es;
        parsec_atomic_add_32b((volatile uint32_t*)&task->taskpool->nb_tasks, -1);
        return PARSEC_HOOK_RETURN_DONE;
    }

    parsec_hook_return_t release_madness_task (parsec_execution_stream_t *es, 
                                              parsec_task_t *task)
    {
        PoolTaskInterface *c = ((PoolTaskInterface **)task->locals)[0];
        delete(c);
        return PARSEC_HOOK_RETURN_DONE;
    }

    const parsec_task_class_t* PoolTaskInterface::func = &madness_function;

    parsec_hook_return_t pointer_call(parsec_execution_stream_t *es, 
                                     parsec_task_t *task)
    {
        PoolTaskInterface *c = ((PoolTaskInterface **)task->locals)[0];
        c->run(TaskThreadEnv(1, 0, 0));
        return PARSEC_HOOK_RETURN_DONE;
    }


  extern "C"{

#include <stdio.h>

        static uint64_t RUN_hash(const parsec_taskpool_t *handle, const assignment_t *a)
        {
            (void)a; (void)handle;
            return 0;
        }

        static parsec_hook_return_t empty_hook (parsec_execution_stream_t *es, 
                                               parsec_task_t *task)
        {
            return PARSEC_HOOK_RETURN_DONE;
        }

    /* g++ requires the initialization of all the fields */
        static const __parsec_chore_t __RUN_chores[] = {
            {type : PARSEC_DEV_CPU,
             here : NULL,
             evaluate : NULL,
             hook : pointer_call,
             dyld : NULL,
             dyld_fn : NULL},
            {type : PARSEC_DEV_NONE,
             here : NULL,
             evaluate : NULL,
             hook : (parsec_hook_t *) NULL,
             dyld : NULL,
             dyld_fn : NULL},
        };

        const parsec_task_class_t madness_function = {
            name : "RUN",
            flags : PARSEC_HAS_IN_IN_DEPENDENCIES | PARSEC_USE_DEPS_MASK,
            task_class_id : 0,
            nb_flows : 0,
            nb_parameters : 0,
            nb_locals : 0,
            dependencies_goal : 0,
            params : {NULL,NULL,NULL, NULL},
            locals : {NULL,NULL,NULL, NULL},
            in : {NULL, NULL, NULL, NULL},
            out : {NULL, NULL, NULL, NULL},
            priority : NULL,
            initial_data : NULL,
            final_data : NULL,
            data_affinity : NULL,
            key_generator : (parsec_functionkey_fn_t *) RUN_hash,
            key_functions : NULL,
            make_key : NULL,
#if defined(PARSEC_SIM)
            sim_cost_fct : (parsec_sim_cost_fct_t*) NULL,
#endif
            get_datatype : (parsec_datatype_lookup_t *) NULL,
            prepare_input : empty_hook,
            incarnations : __RUN_chores,
            prepare_output : (parsec_hook_t *) NULL,
            find_deps : (parsec_find_dependency_fn_t*)NULL,
            iterate_successors : (parsec_traverse_function_t *) NULL,
            iterate_predecessors : (parsec_traverse_function_t *) NULL,
            release_deps : (parsec_release_deps_t *) NULL,
            complete_execution : complete_madness_task_execution,
            new_task : (parsec_new_task_function_t*) NULL,
            release_task : release_madness_task, //object delete,
            fini : (parsec_hook_t *) NULL,
        };

        const parsec_task_class_t* madness_function_array[]= {&(madness::madness_function), NULL};

        parsec_taskpool_t madness_handle = {
            super : { 0x0, },
            taskpool_id : 0,
            nb_tasks : 0,
            taskpool_type : 0,
            devices_mask : PARSEC_DEVICES_ALL,
            initial_number_tasks : 0,
            priority : 0,
            nb_pending_actions : 1,
            context : NULL,
            startup_hook : NULL,
            task_classes_array : madness_function_array,
#if defined(PARSEC_PROF_TRACE)
            profiling_array : NULL,
#endif  /* defined(PARSEC_PROF_TRACE) */
            on_enqueue : NULL,
            on_enqueue_data : NULL,
            on_complete : NULL,
            on_complete_data : NULL,
            update_nb_runtime_task : fake_update_runtime_task,
            destructor : NULL,
            dependencies_array : NULL,
            repo_array : NULL
        };
    }
}
#endif
