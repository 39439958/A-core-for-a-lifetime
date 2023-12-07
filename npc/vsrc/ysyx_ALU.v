module ysyx_ALU(
    input [31:0] SrcA,SrcB,
    input [3:0]  func,
    output reg [31:0] ALUout
);
    wire signed [31:0] signed_a;
    wire signed [31:0] signed_b;

    assign signed_a = SrcA;
    assign signed_b = SrcB;

    always@(*)
    begin
    case(func)
        4'b0000: ALUout = signed_a + signed_b;
        4'b1000: ALUout = signed_a - signed_b;
        4'b0001: ALUout = signed_a << signed_b[4:0];
        4'b0010: ALUout = (signed_a < signed_b ? 1 : 0);
        4'b0011: begin
            case({signed_a[31],signed_b[31]})
                2'b00: ALUout = signed_a < signed_b ? 1 : 0;
                2'b01: ALUout = 1;
                2'b10: ALUout = 0;
                2'b11: ALUout = signed_a < signed_b ? 1 : 0;
                default: ALUout = 0;
            endcase
        end
        4'b0100: ALUout = signed_a ^ signed_b;
        4'b0101: ALUout = signed_a >> signed_b[4:0];
        4'b1101: ALUout = signed_a >>> signed_b[4:0];
        4'b0110: ALUout = signed_a | signed_b;
        4'b0111: ALUout = signed_a & signed_b;
        4'b1110: ALUout = signed_b;
        default: ALUout = 0;	
        endcase
    end 

endmodule
