#ifndef COMMON_DATA_STRUCTURES_H
#define COMMON_DATA_STRUCTURES_H
#include <vector>
#include <stack>
#include <queue>
#include <string>
using namespace std;

namespace mr::sdmp::commom {

template <typename T>
class TreeNode{
public:
    TreeNode(T v):value_(v){}
    T value_;
    std::vector<TreeNode<T>> childrens_;
};

class TreeRecursive{
public:
    template<typename T> static void recurTree(TreeNode<T> root,std::vector<std::vector<T>>& result) {
            std::vector<T> path;
            path.push_back(root.value_);
            findPath(root, result, path);
        }
    template<typename T> static void findPath(TreeNode<T>& node,std::vector<std::vector<T>>& result,std::vector<T>& parent_path){
        if(node.childrens_.empty()){
            result.push_back(parent_path);
            return;
        }
        for (int i = 0; i < node.childrens_.size(); i++) {
            auto& child = node.childrens_[i];
            std::vector<T> current_path(parent_path);
            current_path.push_back(child.value_);
            findPath(child,result, current_path);
        }
    }

    template<typename T> static std::vector<std::vector<TreeNode<T>>> dfsTree(TreeNode<T> root) {
        std::stack<TreeNode<T>> nodeStack;
        std::stack<vector<TreeNode<T>>> pathStack ;
        std::vector<std::vector<TreeNode<T>>> result;
        nodeStack.push(root);
        vector<TreeNode<T>> arrayList;
        arrayList.push_back(root);
        pathStack.push(arrayList);

        while (!nodeStack.empty()) {
            auto curNode = nodeStack.top();nodeStack.pop();
            auto curPath = pathStack.top();pathStack.pop();

            if (curNode.childrens_.size() <= 0) {
                result.push_back(curPath);
            } else {
                int childSize = curNode.childrens_.size();
                for (int i = childSize - 1; i >= 0; i--) {
                    auto node = curNode.childrens_[i];
                    nodeStack.push(node);
                    std::vector<TreeNode<T>> list(curPath);
                    list.push_back(node);
                    pathStack.push(list);
                }
            }
        }
        return result;
    }

    template<typename T> static std::vector<std::vector<TreeNode<T>>> bfsTree(TreeNode<T> root) {
            std::queue<TreeNode<T>> nodeQueue;
            std::queue<vector<TreeNode<T>>> qstr;
            std::vector<vector<TreeNode<T>>> result;
            nodeQueue.push(root);
            std::vector<TreeNode<T>> arrayList;
            qstr.push(arrayList);

            while (!nodeQueue.empty()) {
                TreeNode<T> curNode = nodeQueue.front();nodeQueue.pop();
                vector<TreeNode<T>> curList = qstr.front();qstr.pop();

                if (curNode.childrens_.size() <= 0) {
                    curList.push_back(curNode);
                    result.push_back(curList);
                } else {
                    for (int i = 0; i < curNode.childrens_.size(); i++) {
                        TreeNode treeNode = curNode.childrens_[i];
                        nodeQueue.push(treeNode);
                        std::vector<TreeNode<T>> list(curList);
                        list.push_back(curNode);
                        qstr.push(list);
                    }
                }
            }

            return result;
        }
};


static void test(){

    TreeNode<string> root("A");
    // 第二层
    root.childrens_.push_back(TreeNode<string>("B"));
    root.childrens_.push_back(TreeNode<string>("C"));
    // 第三层
    root.childrens_[0].childrens_.push_back(TreeNode<string>("D"));
    root.childrens_[0].childrens_.push_back(TreeNode<string>("E"));
    root.childrens_[1].childrens_.push_back(TreeNode<string>("F"));
    root.childrens_[1].childrens_.push_back(TreeNode<string>("H"));
    root.childrens_[1].childrens_.push_back(TreeNode<string>("G"));
    // 第四层
    root.childrens_[0].childrens_[1].childrens_.push_back(TreeNode<string>("I"));

    TreeNode<string>  end("END");
    root.childrens_[0].childrens_[1].childrens_.push_back(end);
    root.childrens_[1].childrens_[1].childrens_.push_back(end);

    std::vector<std::vector<string>> result;
    TreeRecursive::recurTree<string>(root,result);
    for (auto& item : result) {
        fprintf(stderr,"recur:");
        for (auto i : item) {
            fprintf(stderr,"%s ",i.c_str());
        }
        fprintf(stderr,"\n");
    }

    auto result2 = TreeRecursive::dfsTree<string>(root);
    for (auto& item : result2) {
        fprintf(stderr,"stack depth:");
        for (auto i : item) {
            fprintf(stderr,"%s ",i.value_.c_str());
        }
        fprintf(stderr,"\n");
    }

    result2 = TreeRecursive::bfsTree<string>(root);
    for (auto& item : result2) {
        fprintf(stderr,"stack width:");
        for (auto i : item) {
            fprintf(stderr,"%s ",i.value_.c_str());
        }
        fprintf(stderr,"\n");
    }
}
}

#endif // COMMON_DATA_STRUCTURES_H
