#include <tree_recursive.h>

using namespace mr::sdmp::commom;
int main(int argc,char** argv){

    TreeNode<string> root("R");
    // 第二层
    root.childrens_.push_back(TreeNode<string>("R-1"));
    root.childrens_.push_back(TreeNode<string>("R-2"));
    // 第三层
    root.childrens_[0].childrens_.push_back(TreeNode<string>("R-1-1"));
    root.childrens_[0].childrens_.push_back(TreeNode<string>("R-1-2"));
    root.childrens_[1].childrens_.push_back(TreeNode<string>("R-2-1"));
    root.childrens_[1].childrens_.push_back(TreeNode<string>("R-2-2"));
    root.childrens_[1].childrens_.push_back(TreeNode<string>("R-2-3"));
    // 第四层
    root.childrens_[0].childrens_[1].childrens_.push_back(TreeNode<string>("R-1-2-1"));

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
