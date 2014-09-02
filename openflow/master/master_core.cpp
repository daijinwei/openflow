// Copyright (c) 2014, OpenFlow
// Author: RenZhen<renzhen@163.com>
// Created: 2014-06-29
// Description:
//
#include <boost/format.hpp>
#include <glog/logging.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/singleton.hpp>
#include "../common/table.h"
#include "../common/database.h"
#include "../common/dataset.h"
#include "master_core.h"

namespace openflow { namespace master {

bool CMasterCore::fetch_job(const int32_t job_id)
{
    common::CDataSet &ds = boost::serialization::singleton<common::CDataSet>::get_mutable_instance();
    common::CDatabase *db = ds.new_database(common::DB_SQLITE, "openflow.db");

    //SQLite database file path just like connect string.
    db->set_connect_str("../web/database/openflow.db");
    //open connect handler
    db->open();

    //FIXME: table name should not be fixed.
    common::CTable *table = db->new_table("tbJobs");
    if (!table)
    {
        LOG(ERROR) << "Fail to create table.";
        //FIXME: ugly code.
        db->close();        //close database connection.
        delete table;
        delete db;
        return false;
    }

    std::string sql = boost::str(boost::format("SELECT * FROM tbJobs WHERE job_id=%d;")
                                  % job_id);
    std::vector<std::string> rows;
    int num_rows, num_cols;

    num_rows  = table->query(sql, rows, num_cols);
    if ( num_rows < 0)
    {
        LOG(ERROR) << "Query error.";
         //FIXME: ugly code.
        db->close();        //close database connection.
        delete table;
        delete db;
        return false;
    }

    openflow::job_info info;
    info.job_id = boost::lexical_cast<int32_t>(rows[num_cols + 0]);
    info.job_name = rows[num_cols + 1];
    info.xml_desc = rows[num_cols + 2];
    info.time = rows[num_cols + 3];
    _jobs.insert(std::pair<int32_t, openflow::job_info>(job_id, info));

    db->close();        //close database connection.
    delete table;
    delete db;

    return true;
}

bool CMasterCore::parse_job(const int32_t job_id)
{
    boost::property_tree::ptree pt;
    boost::property_tree::ptree root;

    std::stringstream ss;
    ss << _jobs[job_id].xml_desc;
    boost::property_tree::xml_parser::read_xml(ss, pt);
    root = pt.get_child("job");

    const std::string task_member[] = {"name","description","nodes", "command"};
    std::vector<StTask> tasks;
    StTask task;
    for(boost::property_tree::ptree::iterator it = root.begin(); it != root.end(); it++)
    {
        if(it->first == "shell_process")
        {
            boost::property_tree::ptree pt;
            std::string val;
            boost::property_tree::ptree shell_process = it->second;

            int i=0;
            for(; i < 4; i++)
            {
                pt = shell_process.get_child(task_member[i]);
                val = pt.data();
                LOG(INFO) << "XML elemnets: " << val;

                switch(i)
                {
                case 0:
                    task.name = val;
                    break;
                case 1:
                    task.description = val;
                    break;
                case 2:
                    task.nodes = val;
                    break;
                case 3:
                    task.cmd = val;
                    break;
                }

            }
            tasks.push_back(task);
        }
    }
    _tasks.insert(std::pair<int32_t, std::vector<StTask> >(job_id, tasks));

    return true;
}

void CMasterCore::print_job(const int32_t job_id)
{
    openflow::job_info info = _jobs[job_id];
    LOG(INFO) << "Job ID: " << info.job_id;
    LOG(INFO) << "Job name: " << info.job_name;
    LOG(INFO) << "Job xml description: " << info.xml_desc;
    LOG(INFO) << "time: " << info.time;
}

void CMasterCore::print_tasks(const int32_t job_id)
{
    std::vector<StTask> tasks;
    tasks = _tasks[job_id];

    for(std::vector<StTask>::iterator it = tasks.begin(); it != tasks.end(); it++)
    {
        LOG(INFO) << "Task name: " << it->name;
        LOG(INFO) << "Task description: " << it->description;
        LOG(INFO) << "Task nodes: " << it->nodes;
        LOG(INFO) << "Task command: " << it->cmd;
    }
}
}} //enf namespace openflow::master